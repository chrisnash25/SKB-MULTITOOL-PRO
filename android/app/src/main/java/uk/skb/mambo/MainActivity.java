package uk.skb.mambo;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.ViewGroup;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.ScrollView;
import android.widget.FrameLayout;
import android.webkit.WebViewClient;
import android.webkit.WebResourceError;
import org.json.JSONObject;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import java.nio.charset.StandardCharsets;
import java.util.List;

public class MainActivity extends Activity {
  private static final String DEVICE_URL="http://192.168.4.1/";
  private static final String USB_PERMISSION="uk.skb.mambo.USB_PERMISSION";
  private final Handler handler=new Handler(Looper.getMainLooper());
  private UsbManager manager;
  private volatile UsbSerialPort port;
  private UsbSerialDriver pendingDriver;
  private TextView usbStatus;
  private TextView deviceResponse;
  private Spinner radioMode;
  private WebView web;
  private volatile boolean reading=false;
  private volatile boolean active=true;
  private final Runnable reconnectCheck=new Runnable() {
    @Override public void run() {
      if(!active)return;
      if(port==null)discover(false);
      handler.postDelayed(this,2500);
    }
  };
  private final BroadcastReceiver receiver=new BroadcastReceiver() {
    @Override public void onReceive(Context context,Intent intent) {
      String action=intent.getAction();
      if(USB_PERMISSION.equals(action)) {
        UsbDevice device=intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED,false) && pendingDriver!=null && pendingDriver.getDevice().equals(device)) connect(pendingDriver);
        else announce("USB permission was not granted");
      } else if(UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
        UsbDevice device=intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(port!=null && port.getDriver().getDevice().equals(device)) disconnect("USB disconnected");
      } else if(UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
        handler.postDelayed(()->discover(false),400);
      }
    }
  };
  private void announce(String message) {
    runOnUiThread(()->{
      if(usbStatus!=null)usbStatus.setText(message);
    });
  }
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    manager=(UsbManager)getSystemService(USB_SERVICE);
    IntentFilter filter=new IntentFilter();
    filter.addAction(USB_PERMISSION);filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    registerReceiver(receiver,filter,Context.RECEIVER_NOT_EXPORTED);
    getWindow().setStatusBarColor(Color.rgb(12,15,23));
    getWindow().setNavigationBarColor(Color.rgb(12,15,23));
    LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setBackgroundColor(Color.rgb(12,15,23));
    ScrollView scroll=new ScrollView(this);scroll.setFillViewport(false);
    LinearLayout controls=new LinearLayout(this);controls.setOrientation(LinearLayout.VERTICAL);scroll.addView(controls);
    layout.addView(scroll,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1));
    TextView title=new TextView(this);title.setText("MAMBO OS V3  •  DEVICE CONTROL");title.setTextColor(Color.WHITE);title.setPadding(20,36,12,20);controls.addView(title);
    usbStatus=new TextView(this);usbStatus.setTextColor(Color.WHITE);usbStatus.setPadding(16,12,16,12);usbStatus.setText("USB: disconnected");controls.addView(usbStatus);
    Button usb=new Button(this);usb.setText("Connect USB / request permission");controls.addView(usb);
    Button status=new Button(this);status.setText("USB device status");controls.addView(status);
    Button radio=new Button(this);radio.setText("USB radio diagnostics");controls.addView(radio);
    Button probe=new Button(this);probe.setText("Read-only SPI probe (USB)");controls.addView(probe);
    Button nrf=new Button(this);nrf.setText("nRF24 channel activity (USB)");controls.addView(nrf);
    Button cc=new Button(this);cc.setText("CC1101 RSSI (USB)");controls.addView(cc);
    radioMode=new Spinner(this);
    radioMode.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"none","cc1101","nrf24"}));
    controls.addView(radioMode);
    Button selectRadio=new Button(this);selectRadio.setText("Set selected radio (USB)");controls.addView(selectRadio);
    deviceResponse=new TextView(this);deviceResponse.setTextColor(Color.WHITE);
    deviceResponse.setPadding(16,10,16,10);deviceResponse.setText("Connect USB to view device status");
    deviceResponse.setTextIsSelectable(true);
    controls.addView(deviceResponse);
    Button wifi=new Button(this);wifi.setText("Open Wi-Fi dashboard (requires Mambo Wi-Fi)");controls.addView(wifi);
    web=new WebView(this);web.getSettings().setJavaScriptEnabled(true);web.getSettings().setDomStorageEnabled(true);
    web.setWebViewClient(new WebViewClient(){
      @Override public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
        if(request.isForMainFrame())announce("Dashboard unavailable. Connect phone Wi-Fi to Mambo OS AP, then tap Retry.");
      }
      @Override public boolean shouldOverrideUrlLoading(WebView view,WebResourceRequest request) {
        return !request.getUrl().toString().startsWith(DEVICE_URL);
      }
    });
    web.setVisibility(android.view.View.GONE);
    controls.addView(web,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,520));
    usb.setOnClickListener(v->discover(true));
    status.setOnClickListener(v->send("{\"cmd\":\"status\"}"));
    radio.setOnClickListener(v->send("{\"cmd\":\"radio_status\"}"));
    probe.setOnClickListener(v->send("{\"cmd\":\"radio_probe\"}"));
    nrf.setOnClickListener(v->send("{\"cmd\":\"radio_observe\",\"start_channel\":0,\"count\":8}"));
    cc.setOnClickListener(v->send("{\"cmd\":\"cc1101_rssi\"}"));
    selectRadio.setOnClickListener(v->{
      try {JSONObject cmd=new JSONObject();cmd.put("cmd","radio_select");cmd.put("mode",radioMode.getSelectedItem().toString());send(cmd.toString());}
      catch(Exception e){announce("Invalid radio selection");}
    });
    wifi.setOnClickListener(v->{
      web.setVisibility(android.view.View.VISIBLE);
      web.loadUrl(DEVICE_URL);
      wifi.setText("Retry Wi-Fi dashboard");
    });
    setContentView(layout);
    discover(false);
    handler.postDelayed(reconnectCheck,2500);
  }
  private void discover(boolean prompt) {
    if(!active || port!=null)return;
    List<UsbSerialDriver> drivers=UsbSerialProber.getDefaultProber().findAllDrivers(manager);
    if(drivers.isEmpty()){if(prompt)announce("No supported USB serial device found");return;}
    UsbSerialDriver driver=drivers.get(0);
    pendingDriver=driver;
    if(manager.hasPermission(driver.getDevice()))connect(driver);
    else if(prompt) {
      announce("Please allow USB device access");
      PendingIntent permission=PendingIntent.getBroadcast(this,0,new Intent(USB_PERMISSION).setPackage(getPackageName()),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_MUTABLE);
      manager.requestPermission(driver.getDevice(),permission);
    } else announce("USB device detected. Tap Connect USB to authorise.");
  }
  private synchronized void connect(UsbSerialDriver driver) {
    if(port!=null)return;
    try {
      android.hardware.usb.UsbDeviceConnection connection=manager.openDevice(driver.getDevice());
      if(connection==null){announce("USB connection could not be opened");return;}
      UsbSerialPort candidate=driver.getPorts().get(0);
      try {candidate.open(connection);candidate.setParameters(115200,8,UsbSerialPort.STOPBITS_1,UsbSerialPort.PARITY_NONE);}
      catch(Exception e){try{candidate.close();}catch(Exception ignored){}connection.close();throw e;}
      port=candidate;reading=true;
      announce("Mambo OS connected over USB");
      new Thread(()->{
        byte[] buffer=new byte[1024];StringBuilder line=new StringBuilder();
        while(reading) {
          try {
            UsbSerialPort current=port;if(current==null)break;
            int n=current.read(buffer,500);
            for(int i=0;i<n;i++){
              char c=(char)(buffer[i]&255);
              if(c=='\n'){String message=line.toString();line.setLength(0);if(message.startsWith("{"))handleResponse(message);}
              else if(c!='\r' && line.length()<2048)line.append(c);
            }
          } catch(Exception e){break;}
        }
        handler.post(()->{if(active && port!=null)disconnect("USB connection lost");});
      },"mambo-usb-reader").start();
      send("{\"cmd\":\"status\"}");
    } catch(Exception e){announce("USB connection failed: "+e.getMessage());}
  }
  private void handleResponse(String message) {
    try {
      JSONObject reply=new JSONObject(message);
      if(reply.has("selected_mode")) {
        String mode=reply.optString("selected_mode","none");
        runOnUiThread(()->{
          if(radioMode!=null)for(int i=0;i<radioMode.getCount();i++)
            if(mode.equals(radioMode.getItemAtPosition(i)))radioMode.setSelection(i);
        });
      }
      final String display=formatReply(reply);
      runOnUiThread(()->{if(deviceResponse!=null)deviceResponse.setText(display);});
    } catch(Exception ignored) { announce("Unrecognised USB response"); }
  }
  private String formatReply(JSONObject r) {
    if(r.optBoolean("ok",true)==false)return "Action unavailable: "+r.optString("error","Unknown error")+"\nRadio: "+r.optString("mode",r.optString("selected_mode","none"));
    String mode=r.optString("selected_mode",r.optString("mode",""));
    StringBuilder out=new StringBuilder("Device response");
    if(!mode.isEmpty())out.append("\nRadio: ").append(mode);
    if(r.has("rssi"))out.append("\nRSSI: ").append(r.opt("rssi"));
    if(r.has("transmission_enabled"))out.append("\nTransmission: ").append(r.optBoolean("transmission_enabled")?"enabled":"disabled");
    if(out.toString().equals("Device response"))return r.toString();
    return out.toString();
  }
  private void send(String command) {
    UsbSerialPort current=port;
    if(current==null){announce("Connect USB first");return;}
    new Thread(()->{try{current.write((command+"\n").getBytes(StandardCharsets.UTF_8),1500);}catch(Exception e){announce("USB write failed");}},"mambo-usb-write").start();
  }
  private synchronized void disconnect(String reason) {
    reading=false;
    UsbSerialPort old=port;port=null;
    if(old!=null)try{old.close();}catch(Exception ignored){}
    announce(reason);
    // Periodic reconnectCheck handles later reattachment while app is active.
  }
  @Override protected void onDestroy() {
    active=false;reading=false;
    handler.removeCallbacks(reconnectCheck);
    unregisterReceiver(receiver);
    disconnect("USB closed");
    if(web!=null)web.destroy();
    super.onDestroy();
  }
}
