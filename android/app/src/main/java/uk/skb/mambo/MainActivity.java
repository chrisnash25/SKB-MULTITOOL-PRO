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
import android.view.WindowInsets;
import android.os.Build;
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
import android.widget.EditText;
import android.widget.ImageView;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.widget.FrameLayout;
import android.view.View;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import java.net.HttpURLConnection;
import java.net.URL;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
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
  private LinearLayout appRoot, moduleBody;
  private TextView pageTitle, connectionLabel;
  private String currentPage="Home";
  private static final String[] PAGES={"Home","Wi-Fi","Bluetooth","CC1101 RF","nRF24","Infrared","USB","Settings"};
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
      if(connectionLabel!=null)connectionLabel.setText(message);
    });
  }
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    manager=(UsbManager)getSystemService(USB_SERVICE);
    IntentFilter filter=new IntentFilter();
    filter.addAction(USB_PERMISSION);filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    registerReceiver(receiver,filter,Context.RECEIVER_NOT_EXPORTED);
    getWindow().setStatusBarColor(Color.rgb(2,10,18));
    getWindow().setNavigationBarColor(Color.rgb(2,10,18));
    appRoot=new LinearLayout(this);appRoot.setOrientation(LinearLayout.VERTICAL);appRoot.setBackgroundColor(Color.rgb(2,10,18));
    // Android 15 edge-to-edge: keep the pager clear of gesture/3-button navigation.
    final int baseBottom=dp(8);
    appRoot.setOnApplyWindowInsetsListener((view,insets)->{
      int bottom;
      if(Build.VERSION.SDK_INT>=30) bottom=insets.getInsets(WindowInsets.Type.navigationBars() | WindowInsets.Type.displayCutout()).bottom;
      else bottom=insets.getSystemWindowInsetBottom();
      view.setPadding(0,0,0,Math.max(baseBottom,bottom+dp(8)));
      return insets;
    });
    LinearLayout header=new LinearLayout(this);header.setGravity(android.view.Gravity.CENTER_VERTICAL);header.setPadding(12,24,12,12);
    Button back=button("❮");header.addView(back,new LinearLayout.LayoutParams(56,-2));back.setOnClickListener(v->showPage("Home"));
    pageTitle=new TextView(this);pageTitle.setTextColor(Color.WHITE);pageTitle.setTypeface(null,Typeface.BOLD);pageTitle.setTextSize(20);pageTitle.setGravity(android.view.Gravity.CENTER);
    header.addView(pageTitle,new LinearLayout.LayoutParams(0,-2,1));
    Button menu=button("☰");header.addView(menu,new LinearLayout.LayoutParams(56,-2));menu.setOnClickListener(v->showPage("Home"));
    appRoot.addView(header);
    connectionLabel=new TextView(this);connectionLabel.setTextColor(Color.rgb(75,238,246));connectionLabel.setPadding(20,6,12,12);connectionLabel.setText("USB: disconnected  •  Wi-Fi: not checked");
    appRoot.addView(connectionLabel);
    ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);
    moduleBody=new LinearLayout(this);moduleBody.setOrientation(LinearLayout.VERTICAL);moduleBody.setPadding(16,8,16,20);
    scroll.addView(moduleBody);appRoot.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
    LinearLayout pager=new LinearLayout(this);pager.setGravity(android.view.Gravity.CENTER);
    pager.setMinimumHeight(dp(64));
    Button prev=button("❮ PREV"),next=button("NEXT ❯");
    pager.addView(prev,new LinearLayout.LayoutParams(0,-2,1));pager.addView(next,new LinearLayout.LayoutParams(0,-2,1));appRoot.addView(pager);
    prev.setOnClickListener(v->stepPage(-1));next.setOnClickListener(v->stepPage(1));
    final float[] down={0,0};
    scroll.setOnTouchListener((v,e)->{
      if(e.getAction()==android.view.MotionEvent.ACTION_DOWN){down[0]=e.getX();down[1]=e.getY();}
      if(e.getAction()==android.view.MotionEvent.ACTION_UP){
        float dx=e.getX()-down[0],dy=e.getY()-down[1];
        if(Math.abs(dx)>120&&Math.abs(dx)>Math.abs(dy)*1.7f){stepPage(dx<0?1:-1);return true;}
      }return false;
    });
    setContentView(appRoot);appRoot.requestApplyInsets();showPage("Home");
    discover(false);
    handler.postDelayed(reconnectCheck,2500);
  }

  private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
  private Button button(String label){
    Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setTextColor(Color.WHITE);
    GradientDrawable bg=new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{0xff064354,0xff07121c});
    bg.setCornerRadius(28);bg.setStroke(2,0xff2adcf0);b.setBackground(bg);
    LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2);p.setMargins(5,7,5,7);b.setLayoutParams(p);return b;
  }
  private void addAction(String label,Runnable action){Button b=button(label);moduleBody.addView(b);b.setOnClickListener(v->action.run());}
  private void note(String label){TextView t=new TextView(this);t.setText(label);t.setTextColor(0xffb6dce5);t.setTextSize(15);t.setPadding(10,12,10,12);moduleBody.addView(t);}
  private void stepPage(int delta){int i=java.util.Arrays.asList(PAGES).indexOf(currentPage);showPage(PAGES[Math.max(0,Math.min(PAGES.length-1,i+delta))]);}

  private ImageView artwork(String name){
    ImageView image=new ImageView(this);
    try(InputStream in=getAssets().open("branding/"+name)){Bitmap bitmap=BitmapFactory.decodeStream(in);image.setImageBitmap(bitmap);}
    catch(Exception e){image.setContentDescription("Missing branding image: "+name);}
    image.setScaleType(ImageView.ScaleType.FIT_CENTER);return image;
  }
  private void homeHotspot(FrameLayout layer,String page,int x,int y,int w,int h,int width,int height){
    View tap=new View(this);tap.setContentDescription(page);tap.setClickable(true);
    FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(Math.round(w*width/1024f),Math.round(h*height/1536f));
    lp.leftMargin=Math.round(x*width/1024f);lp.topMargin=Math.round(y*height/1536f);
    layer.addView(tap,lp);tap.setOnClickListener(v->showPage(page));
  }
  private void showArtworkHome(){
    int width=getResources().getDisplayMetrics().widthPixels-Math.round(32*getResources().getDisplayMetrics().density);
    int height=Math.round(width*1.5f);
    FrameLayout layer=new FrameLayout(this);
    ImageView background=artwork("mambo_home.png");background.setScaleType(ImageView.ScaleType.FIT_XY);
    layer.addView(background,new FrameLayout.LayoutParams(-1,-1));
    String[] destinations={"Home","Wi-Fi","Bluetooth","CC1101 RF","nRF24","Infrared","USB","Settings"};
    int[] xs={44,522,44,522,44,522,44,522};
    int[] ys={522,522,734,734,939,939,1132,1132};
    for(int i=0;i<destinations.length;i++)homeHotspot(layer,destinations[i],xs[i],ys[i],457,190,width,height);
    homeHotspot(layer,"Settings",18,18,82,70,width,height);
    homeHotspot(layer,"Wi-Fi",935,18,72,70,width,height);
    homeHotspot(layer,"USB",44,1370,190,90,width,height);
    homeHotspot(layer,"Wi-Fi",789,1370,190,90,width,height);
    layer.setLayoutParams(new LinearLayout.LayoutParams(-1,height));
    moduleBody.addView(layer);
    note("Live connection status appears above. The battery and connection labels inside the artwork are illustrative.");
  }
  private void showPage(String page){
    currentPage=page;pageTitle.setText("MAMBO OS • "+page);moduleBody.removeAllViews();
    if(web!=null && web.getParent()!=null)((ViewGroup)web.getParent()).removeView(web);
    if(!page.equals("Home")){
      ImageView brand=artwork("mambo_cat.png");moduleBody.addView(brand,new LinearLayout.LayoutParams(-1,150));
      ImageView skb=artwork("skb_artwork.png");moduleBody.addView(skb,new LinearLayout.LayoutParams(-1,90));
    }
    switch(page){
      case "Home":
        showArtworkHome();break;
      case "Wi-Fi":
        note("Wi-Fi tools require your phone connected to the Mambo OS access point.");
        addAction("Check Wi-Fi connection",()->request("/api/status","GET",null));
        addAction("Scan networks",()->request("/api/wifi/scan","POST","{}"));
        addAction("View scan results",()->request("/api/wifi/results","GET",null));
        addAction("Signal monitor",()->request("/api/signal/wifi","GET",null));
        addAction("Open full web dashboard",()->openDashboard());
        addAction("Wi-Fi Deauth • Controls",()->showPage("Wi-Fi Deauth"));
        addAction("Evil Portal • Controls",()->showPage("Evil Portal"));
        addAction("Beacon Flood • Controls",()->showPage("Beacon Flood"));break;
      case "Wi-Fi Deauth":
        showUnwiredPanel("Wi-Fi Deauth",true);break;
      case "Evil Portal":
        showUnwiredPanel("Evil Portal",false);break;
      case "Beacon Flood":
        showUnwiredPanel("Beacon Flood",false);break;
      case "Bluetooth":
        addAction("Scan BLE devices",()->request("/api/ble/scan","GET",null));
        addAction("BLE advertising status",()->request("/api/ble/advertise","GET",null));
        addAction("Start BLE advertising",()->request("/api/ble/advertise","POST","{\"enabled\":true}"));
        addAction("Stop BLE advertising",()->request("/api/ble/advertise","POST","{\"enabled\":false}"));
        addAction("BLE tracking settings",()->request("/api/ble/tracking","GET",null));break;
      case "CC1101 RF":
        addAction("Select CC1101 (USB)",()->send("{\"cmd\":\"radio_select\",\"mode\":\"cc1101\"}"));
        addAction("Radio diagnostics (USB)",()->send("{\"cmd\":\"radio_status\"}"));
        addAction("SPI probe (USB)",()->send("{\"cmd\":\"radio_probe\"}"));
        addAction("Read CC1101 RSSI (USB)",()->send("{\"cmd\":\"cc1101_rssi\"}"));
        addAction("Radio Lab (Wi-Fi)",()->openDashboard());break;
      case "nRF24":
        addAction("Select nRF24 (USB)",()->send("{\"cmd\":\"radio_select\",\"mode\":\"nrf24\"}"));
        addAction("SPI probe (USB)",()->send("{\"cmd\":\"radio_probe\"}"));
        addAction("Sample channels (USB)",()->send("{\"cmd\":\"radio_observe\",\"start_channel\":0,\"count\":8}"));
        addAction("Radio Lab (Wi-Fi)",()->openDashboard());break;
      case "Infrared":
        addAction("Saved remote library",()->request("/api/ir/library","GET",null));
        addAction("Samsung TV power",()->request("/api/ir/tv-power","POST","{\"brand\":\"Samsung\",\"confirm\":true}"));
        addAction("LG TV power",()->request("/api/ir/tv-power","POST","{\"brand\":\"LG\",\"confirm\":true}"));
        addAction("Full IR remote controls",()->openDashboard());break;
      case "USB":
        addAction("Connect USB / grant permission",()->discover(true));
        addAction("Device status",()->send("{\"cmd\":\"status\"}"));
        addAction("Radio diagnostics",()->send("{\"cmd\":\"radio_status\"}"));
        addAction("SPI probe",()->send("{\"cmd\":\"radio_probe\"}"));
        addAction("Disconnect USB",()->disconnect("USB disconnected by user"));break;
      case "Settings":
        addAction("Firmware information",()->request("/api/status","GET",null));
        addAction("Radio settings",()->request("/api/mambo/radio","GET",null));
        addAction("Branding & storage (Wi-Fi)",()->openDashboard());break;
    }
    if(page.equals("Home"))return;
    deviceResponse=new TextView(this);deviceResponse.setTextColor(Color.WHITE);deviceResponse.setTextSize(14);deviceResponse.setPadding(12,20,12,12);deviceResponse.setText("Results appear here");deviceResponse.setTextIsSelectable(true);moduleBody.addView(deviceResponse);
  }
  // UI-only placeholders. No RF transmission, credential capture, or device commands.
  private void showUnwiredPanel(String title,boolean includeTarget){
    note(title+" • Interface only. Firmware integration is not installed.");
    if(includeTarget){
      EditText target=new EditText(this);target.setSingleLine(true);
      target.setHint("Target identifier (not connected)");target.setTextColor(Color.WHITE);
      target.setHintTextColor(0xff92b2bd);moduleBody.addView(target);
    }
    if(title.equals("Beacon Flood")){
      EditText names=new EditText(this);names.setHint("SSID names (one per line)");
      names.setMinLines(3);names.setTextColor(Color.WHITE);names.setHintTextColor(0xff92b2bd);
      moduleBody.addView(names);
    }
    if(title.equals("Evil Portal")){
      EditText portalName=new EditText(this);portalName.setSingleLine(true);
      portalName.setHint("Portal display name");portalName.setTextColor(Color.WHITE);
      portalName.setHintTextColor(0xff92b2bd);moduleBody.addView(portalName);
    }
    addAction("START",()->unwiredNotice(title));
    addAction("STOP",()->unwiredNotice(title));
    addAction("STATUS",()->unwiredNotice(title));
    addAction("Back to Wi-Fi",()->showPage("Wi-Fi"));
  }
  private void unwiredNotice(String title){
    if(deviceResponse!=null)deviceResponse.setText(title+": controls are not connected to firmware. No action was transmitted.");
  }
  private void openDashboard(){
    if(web==null){web=new WebView(this);web.getSettings().setJavaScriptEnabled(true);web.getSettings().setDomStorageEnabled(true);}
    moduleBody.removeAllViews();note("Connect your phone Wi-Fi to Mambo OS before using the dashboard.");
    moduleBody.addView(web,new LinearLayout.LayoutParams(-1,1000));web.loadUrl(DEVICE_URL);
  }
  private void request(String endpoint,String method,String body){
    final TextView output=deviceResponse;
    if(output!=null)output.setText("Connecting to Mambo OS Wi-Fi...");
    new Thread(()->{HttpURLConnection c=null;try{
      c=(HttpURLConnection)new URL("http://192.168.4.1"+endpoint).openConnection();c.setConnectTimeout(3500);c.setReadTimeout(4500);c.setRequestMethod(method);
      if(body!=null){c.setDoOutput(true);c.setRequestProperty("Content-Type","application/json");c.getOutputStream().write(body.getBytes(StandardCharsets.UTF_8));}
      InputStream in=(c.getResponseCode()<400?c.getInputStream():c.getErrorStream());
      ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buf=new byte[1024];int n;while((n=in.read(buf))!=-1&&out.size()<20000)out.write(buf,0,n);
      String result=out.toString("UTF-8");runOnUiThread(()->{if(output!=null)output.setText(result);connectionLabel.setText("Wi-Fi: connected • USB: "+(port==null?"disconnected":"connected"));});
    }catch(Exception e){runOnUiThread(()->{if(output!=null)output.setText("Wi-Fi unavailable. Connect phone to the Mambo OS access point.\n"+e.getMessage());});}
    finally{if(c!=null)c.disconnect();}
    },"mambo-http").start();
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
    if(current==null){announce("Connect USB first");if(deviceResponse!=null)deviceResponse.setText("USB not connected. Open USB Tools and tap Connect USB.");return;}
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
    if(web!=null){if(web.getParent()!=null)((ViewGroup)web.getParent()).removeView(web);web.destroy();}
    super.onDestroy();
  }
}
