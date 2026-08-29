package com.rtsoft.growtopia;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Bitmap;
import android.util.Log;
import android.view.View;
import android.webkit.JavascriptInterface;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.RelativeLayout;

public class WebViewManager {
    private static final String TAG = "WebViewManager";
    Activity mainActivity;
    WebView webView;

    private static native void nativeOnErrorOccurred(int errorCode, String description, String url);
    private static native void nativeOnPageContent(String content);
    private static native void nativeOnPageLoaded(String url);
    private static native void nativeOnScriptCall(String data);

    public WebViewManager(Activity activity) {
        mainActivity = activity;
    }

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    public void CreateWebView(final int x, final int y, final int width, final int height) {
        mainActivity.runOnUiThread(() -> {
            webView = new WebView(mainActivity);
            WebSettings settings = webView.getSettings();
            settings.setJavaScriptEnabled(true);
            settings.setDomStorageEnabled(true);
            settings.setLoadWithOverviewMode(true);
            settings.setUseWideViewPort(true);
            webView.setWebViewClient(new WebViewClientImpl());
            webView.addJavascriptInterface(new WebViewJavascriptInterface(), "Android");

            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(width, height);
            params.leftMargin = x;
            params.topMargin = y;
            webView.setLayoutParams(params);

            SharedActivity.app.mViewGroup.addView(webView);
            Log.d(TAG, "WebView created at (" + x + "," + y + ") size " + width + "x" + height);
        });
    }

    public void LoadURL(final String url) {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.loadUrl(url);
            }
        });
    }

    public void LoadHTML(final String html, final String baseUrl) {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.loadDataWithBaseURL(baseUrl, html, "text/html", "utf-8", null);
            }
        });
    }

    public void ExecuteScript(final String script) {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.evaluateJavascript(script, null);
            }
        });
    }

    public void SetVisible(final boolean visible) {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
            }
        });
    }

    public void Destroy() {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.loadUrl("about:blank");
                webView.clearHistory();
                webView.clearCache(true);
                if (webView.getParent() != null) {
                    SharedActivity.app.mViewGroup.removeView(webView);
                }
                webView.destroy();
                webView = null;
                Log.d(TAG, "WebView destroyed");
            }
        });
    }

    public void GoBack() {
        mainActivity.runOnUiThread(() -> {
            if (webView != null && webView.canGoBack()) {
                webView.goBack();
            }
        });
    }

    public void Reload() {
        mainActivity.runOnUiThread(() -> {
            if (webView != null) {
                webView.reload();
            }
        });
    }

    private class WebViewClientImpl extends WebViewClient {
        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {
            super.onPageStarted(view, url, favicon);
            Log.d(TAG, "onPageStarted: " + url);
        }

        @Override
        public void onPageFinished(WebView view, String url) {
            super.onPageFinished(view, url);
            Log.d(TAG, "onPageFinished: " + url);
            nativeOnPageLoaded(url != null ? url : "");
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
            super.onReceivedError(view, request, error);
            if (request.isForMainFrame()) {
                String url = request.getUrl() != null ? request.getUrl().toString() : "";
                String desc = error.getDescription() != null ? error.getDescription().toString() : "";
                Log.e(TAG, "onReceivedError: " + desc + " url=" + url);
                nativeOnErrorOccurred(error.getErrorCode(), desc, url);
                view.loadUrl("about:blank");
            }
        }
    }

    private class WebViewJavascriptInterface {
        @JavascriptInterface
        public void onScriptCall(String data) {
            Log.d(TAG, "onScriptCall: " + data);
            nativeOnScriptCall(data != null ? data : "");
        }

        @JavascriptInterface
        public void onPageContent(String content) {
            Log.d(TAG, "onPageContent received");
            nativeOnPageContent(content != null ? content : "");
        }
    }
}
