package uk.skb.mambo;
import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceRequest;
import android.widget.LinearLayout;
import android.widget.Button;
import android.widget.TextView;
import android.graphics.Color;
import android.view.ViewGroup;

public class MainActivity extends Activity {
  private static final String DEVICE_URL="http://192.168.4.1/";
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setBackgroundColor(Color.rgb(12,15,23));
    TextView title=new TextView(this);title.setText("MAMBO OS V3  |  Device dashboard");title.setTextColor(Color.WHITE);title.setPadding(20,22,12,22);layout.addView(title);
    Button reload=new Button(this);reload.setText("Reconnect to M5StickC Plus2");layout.addView(reload);
    WebView web=new WebView(this);web.getSettings().setJavaScriptEnabled(true);web.getSettings().setDomStorageEnabled(true);
    web.setWebViewClient(new WebViewClient(){
      @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
        String url=request.getUrl().toString();
        return !(url.startsWith(DEVICE_URL));
      }
    });
    layout.addView(web,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1));
    reload.setOnClickListener(v->web.loadUrl(DEVICE_URL));
    setContentView(layout);web.loadUrl(DEVICE_URL);
  }
}
