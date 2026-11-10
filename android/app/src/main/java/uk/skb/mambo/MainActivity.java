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
import android.speech.tts.TextToSpeech;
import android.view.ViewGroup;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Locale;

public class MainActivity extends Activity {
  private static final String DEVICE_URL="http://192.168.4.1/";
  private static final String USB_PERMISSION="uk.skb.mambo.USB_PERMISSION";
  private final Handler handler=new Handler(Looper.getMainLooper());
  private UsbManager manager;
  private UsbSerialPort port;
  private UsbSerialDriver pendingDriver;
  private TextToSpeech voice;
  private TextView usbStatus;
  private WebView web;
  private volatile boolean reading=false;
  private boolean active=true;
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
        handler.postDelayed(()->discover(true),400);
      }
    }
  };
  private void announce(String message) {
    runOnUiThread(()->{
      if(usbStatus!=null)usbStatus.setText(message);
      if(voice!=null)voice.speak(message,TextToSpeech.QUEUE_FLUSH,null,"mambo-usb");
    });
  }
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    manager=(UsbManager)getSystemService(USB_SERVICE);
    voice=new TextToSpeech(this,status->{if(status==TextToSpeech.SUCCESS)voice.setLanguage(Locale.UK);});
    IntentFilter filter=new IntentFilter();
    filter.addAction(USB_PERMISSION);filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    registerReceiver(receiver,filter,Context.RECEIVER_NOT_EXPORTED);
    LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setBackgroundColor(Color.rgb(12,15,23));
    TextView title=new TextView(this);title.setText("MAMBO OS V3 | Wi-Fi + USB");title.setTextColor(Color.WHITE);title.setPadding(20,20,12,20);layout.addView(title);
    usbStatus=new TextView(this);usbStatus.setTextColor(Color.WHITE);usbStatus.setPadding(16,12,16,12);usbStatus.setText("USB not connected");layout.addView(usbStatus);
    Button usb=new Button(this);usb.setText("Connect USB / request permission");layout.addView(usb);
    Button status=new Button(this);status.setText("USB device status");layout.addView(status);
    Button radio=new Button(this);radio.setText("USB radio status");layout.addView(radio);
    Button wifi=new Button(this);wifi.setText("Open Wi-Fi dashboard");layout.addView(wifi);
    web=new WebView(this);web.getSettings().setJavaScriptEnabled(true);web.getSettings().setDomStorageEnabled(true);
    web.setWebViewClient(new WebViewClient(){
      @Override public boolean shouldOverrideUrlLoading(WebView view,WebResourceRequest request) {
        return !request.getUrl().toString().startsWith(DEVICE_URL);
      }
    });
    layout.addView(web,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1));
    usb.setOnClickListener(v->discover(true));
    status.setOnClickListener(v->send("{\\"cmd\\":\\"status\\"}"));
    radio.setOnClickListener(v->send("{\\"cmd\\":\\"radio_status\\"}"));
    wifi.setOnClickListener(v->web.loadUrl(DEVICE_URL));
    setContentView(layout);web.loadUrl(DEVICE_URL);
    discover(false);
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
              if(c=='\\n'){String message=line.toString();line.setLength(0);if(message.startsWith("{"))announce("Device: "+message);}
              else if(c!='\\r' && line.length()<2048)line.append(c);
            }
          } catch(Exception e){break;}
        }
        handler.post(()->{if(active && port!=null)disconnect("USB connection lost");});
      },"mambo-usb-reader").start();
      send("{\\"cmd\\":\\"status\\"}");
    } catch(Exception e){announce("USB connection failed: "+e.getMessage());}
  }
  private void send(String command) {
    UsbSerialPort current=port;
    if(current==null){announce("Connect USB first");return;}
    new Thread(()->{try{current.write((command+"\\n").getBytes(StandardCharsets.UTF_8),1500);}catch(Exception e){announce("USB write failed");}},"mambo-usb-write").start();
  }
  private synchronized void disconnect(String reason) {
    reading=false;
    UsbSerialPort old=port;port=null;
    if(old!=null)try{old.close();}catch(Exception ignored){}
    announce(reason);
    if(active)handler.postDelayed(()->discover(false),1000);
  }
  @Override protected void onDestroy() {
    active=false;reading=false;
    unregisterReceiver(receiver);
    disconnect("USB closed");
    if(web!=null)web.destroy();
    if(voice!=null){voice.stop();voice.shutdown();}
    super.onDestroy();
  }
}
