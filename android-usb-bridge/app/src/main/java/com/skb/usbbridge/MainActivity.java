package com.skb.usbbridge;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.graphics.Color;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import org.json.JSONObject;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends Activity {
    private static final String PERMISSION_ACTION = "com.skb.usbbridge.USB_PERMISSION";
    private UsbManager usbManager;
    private UsbDeviceConnection usbConnection;
    private volatile UsbSerialPort port;
    private volatile boolean listening;
    private Thread readThread;
    private final Object writeLock = new Object();
    private final ExecutorService writer = Executors.newSingleThreadExecutor();
    private WebView web;
    private TextView status;
    private UsbSerialDriver pendingDriver;
    private boolean pageLoaded;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (PERMISSION_ACTION.equals(action)) {
                boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
                if (granted && pendingDriver != null) openSerial(pendingDriver);
                else setStatus("USB permission denied");
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (device != null && usbConnection != null) {
                    disconnect();
                    setStatus("USB disconnected");
                }
            }
        }
    };

    @SuppressLint("SetJavaScriptEnabled")
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(11, 11, 13));
        getWindow().setNavigationBarColor(Color.rgb(11, 11, 13));
        usbManager = (UsbManager) getSystemService(USB_SERVICE);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.rgb(10, 10, 11));
        root.setPadding(8, 8, 8, 0);
        setContentView(root);

        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setTextSize(12);
        status.setText("USB: disconnected");
        bar.addView(status, new LinearLayout.LayoutParams(0, 54, 1f));
        Button connect = new Button(this);
        connect.setText("CONNECT USB");
        connect.setTextSize(11);
        connect.setOnClickListener(v -> connectUsb());
        bar.addView(connect, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, 54));
        root.addView(bar);

        web = new WebView(this);
        web.setBackgroundColor(Color.rgb(10, 10, 11));
        web.getSettings().setJavaScriptEnabled(true);
        web.getSettings().setDomStorageEnabled(false);
        web.getSettings().setAllowFileAccess(true);
        web.getSettings().setAllowFileAccessFromFileURLs(false);
        web.getSettings().setAllowUniversalAccessFromFileURLs(false);
        web.addJavascriptInterface(new UsbCommands(), "SKB");
        web.setWebViewClient(new WebViewClient(){
            @Override public void onPageFinished(WebView view, String url) {
                pageLoaded=true;
                if(port != null) evaluate("window.skbConnected && window.skbConnected(true)");
            }
            @Override public boolean shouldOverrideUrlLoading(WebView view, android.webkit.WebResourceRequest request) {
                // Local application only. Do not navigate to remote untrusted scripts.
                return !request.getUrl().toString().startsWith("file:///android_asset/");
            }
        });
        root.addView(web, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));
        web.loadUrl("file:///android_asset/index.html");

        IntentFilter filter = new IntentFilter();
        filter.addAction(PERMISSION_ACTION);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        if(Build.VERSION.SDK_INT >= 33) registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED);
        else registerReceiver(receiver, filter);
    }

    private void setStatus(String value) {
        runOnUiThread(() -> { if(status != null) status.setText(value); });
    }

    private void evaluate(String javascript) {
        runOnUiThread(() -> { if(web != null && pageLoaded) web.evaluateJavascript(javascript, null); });
    }

    private void sendError(int id, String message) {
        try {
            JSONObject payload = new JSONObject();
            payload.put("id", id);payload.put("ok", false);payload.put("error", message);
            deliver(payload.toString());
        } catch(Exception ignored) { }
    }

    private void deliver(String line) {
        evaluate("window.skbReply && window.skbReply(" + JSONObject.quote(line) + ");");
    }

    private void connectUsb() {
        if(port != null){ disconnect(); setStatus("USB: disconnected"); return; }
        List<UsbSerialDriver> drivers = UsbSerialProber.getDefaultProber().findAllDrivers(usbManager);
        if(drivers.isEmpty()){
            // The StickC Plus2 CH9102 is normally recognised as CDC ACM.
            for(UsbDevice d : usbManager.getDeviceList().values()){
                if(d.getVendorId()==0x1a86){drivers.add(new CdcAcmSerialDriver(d));}
            }
        }
        if(drivers.isEmpty()){setStatus("No USB serial device found — connect your OTG cable");return;}
        UsbSerialDriver selected=drivers.get(0);
        for(UsbSerialDriver d:drivers) if(d.getDevice().getVendorId()==0x1a86){selected=d;break;}
        pendingDriver=selected;
        if(usbManager.hasPermission(selected.getDevice())) openSerial(selected);
        else{
            setStatus("Approve Android USB access");
            Intent permission = new Intent(PERMISSION_ACTION).setPackage(getPackageName());
            PendingIntent pi=PendingIntent.getBroadcast(this,0,permission,PendingIntent.FLAG_MUTABLE|PendingIntent.FLAG_UPDATE_CURRENT);
            usbManager.requestPermission(selected.getDevice(),pi);
        }
    }

    private void openSerial(UsbSerialDriver driver) {
        try{
            disconnect();
            usbConnection=usbManager.openDevice(driver.getDevice());
            if(usbConnection==null)throw new Exception("USB permission/open failed");
            UsbSerialPort p=driver.getPorts().get(0);
            p.open(usbConnection);
            p.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            port=p;
            listening=true;
            setStatus("USB: connected • 115200 baud");
            evaluate("window.skbConnected && window.skbConnected(true);");
            readThread=new Thread(this::readLoop,"skb-usb-read");
            readThread.start();
        } catch(Exception e){disconnect();setStatus("USB error: "+e.getMessage());}
    }

    private void readLoop(){
        byte[] bytes=new byte[4096];
        StringBuilder line=new StringBuilder();
        try{
            while(listening){
                UsbSerialPort p=port;if(p==null)break;
                int count=p.read(bytes,500);
                for(int i=0;i<count;i++){
                    int b=bytes[i]&255;
                    if(b==10){
                        String message=line.toString().trim();line.setLength(0);
                        if(message.startsWith("{")&&message.contains("\"id\""))deliver(message);
                    }else if(b!=13){
                        if(line.length()<131072)line.append((char)b);
                        else line.setLength(0);
                    }
                }
            }
        }catch(Exception e){if(listening){setStatus("USB read error: "+e.getMessage());}}
    }

    private void disconnect(){
        listening=false;
        UsbSerialPort p=port;port=null;
        try{if(p!=null)p.close();}catch(Exception ignored){}
        try{if(usbConnection!=null)usbConnection.close();}catch(Exception ignored){}
        usbConnection=null;
        evaluate("window.skbConnected && window.skbConnected(false);");
    }

    public final class UsbCommands {
        @JavascriptInterface public boolean isConnected(){return port!=null;}
        @JavascriptInterface public void command(String raw){
            writer.execute(()->{
                int id=-1;
                try{
                    JSONObject request=new JSONObject(raw);id=request.optInt("id",-1);
                    UsbSerialPort p=port;
                    if(p==null){sendError(id,"USB not connected");return;}
                    byte[] bytes=(raw+"\n").getBytes(StandardCharsets.UTF_8);
                    if(bytes.length>510){sendError(id,"Request too large for USB serial protocol");return;}
                    synchronized(writeLock){p.write(bytes,3000);}
                }catch(Exception e){sendError(id,"USB send error: "+e.getMessage());}
            });
        }
    }

    @Override public void onBackPressed(){
        if(web!=null){evaluate("window.skbPrevPage && window.skbPrevPage()");}
        else super.onBackPressed();
    }
    @Override protected void onDestroy(){
        disconnect();
        try{unregisterReceiver(receiver);}catch(Exception ignored){}
        writer.shutdownNow();
        if(web!=null)web.destroy();
        super.onDestroy();
    }
}
