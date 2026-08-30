package com.rtsoft.growtopia;

import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Looper;
import android.util.Log;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.JavascriptInterface;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebStorage;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.RelativeLayout;
import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WebViewManager {
    private static String originalURL;
    private Activity baseActivity;
    private final ExecutorService webViewWorkExecutor;
    boolean allowExternalLinks = true;
    private WebView webView = null;

    private interface WebViewCalbackListener {
        void OnError(int i);
        void OnPageLoaded(String str);
    }

    native void nativeOnErrorOccurred(int i);
    native void nativeOnPageContent(String str);
    native void nativeOnPageLoaded(String str);
    native void nativeOnScriptCall(String str, String str2);

    public void MoveView(int i) {
        WebView wv = this.webView;
        if (wv == null) return;
        ObjectAnimator anim = ObjectAnimator.ofFloat(wv, "translationY", (-i) / 2.0f);
        anim.setDuration(200L);
        anim.start();
    }

    public WebViewManager(Activity activity) {
        this.baseActivity = null;
        ExecutorService executor = Executors.newSingleThreadExecutor();
        this.webViewWorkExecutor = executor;
        this.baseActivity = activity;
        executor.execute(() -> {
            try {
                clearWebViewDirectories();
            } catch (Exception e) {
                Log.e("WebView", "WebView cleanup failed", e);
            }
        });
    }

    public void destroy() {
        this.webViewWorkExecutor.shutdown();
    }

    public boolean IsVisible() {
        WebView wv = this.webView;
        return wv != null && wv.getVisibility() == 0;
    }

    private void ClearCookieWebData() {
        CookieManager cm = CookieManager.getInstance();
        cm.removeAllCookies(null);
        cm.flush();
        WebStorage.getInstance().deleteAllData();
    }

    private void DestroyWebView() {
        if (this.webView == null) return;
        Log.i(SharedActivity.PackageName, "Destroying WebView.");
        ViewGroup parent = (ViewGroup) this.webView.getParent();
        if (parent != null) parent.removeView(this.webView);
        this.webView.stopLoading();
        this.webView.loadUrl("about:blank");
        this.webView.clearHistory();
        this.webView.clearCache(true);
        this.webView.clearFormData();
        this.webView.removeJavascriptInterface("NativeApp");
        this.webView.destroy();
        this.webView = null;
        ClearCookieWebData();
    }

    private synchronized void ShowWebView() {
        if (Looper.getMainLooper().getThread() != Thread.currentThread()) return;
        if (this.webView == null) {
            WebView wv = new WebView(this.baseActivity);
            this.webView = wv;
            wv.setWebViewClient(new WebViewClientImpl(this.baseActivity, new WebViewCalbackListener() {
                @Override
                public void OnError(int i) {
                    WebViewManager.this.nativeOnErrorOccurred(i);
                }
                @Override
                public void OnPageLoaded(String str) {
                    WebViewManager.this.nativeOnPageLoaded(str);
                }
            }));
            WebSettings settings = wv.getSettings();
            settings.setJavaScriptEnabled(true);
            settings.setLoadsImagesAutomatically(true);
            settings.setDomStorageEnabled(true);
            wv.setBackgroundColor(0);
            wv.setScrollBarStyle(0);
            wv.setLayoutParams(new RelativeLayout.LayoutParams(-1, -1));
            wv.addJavascriptInterface(new WebViewJavascriptInterface(this), "NativeApp");
            ((SharedActivity) this.baseActivity).mViewGroup.addView(wv);
        }
        this.webView.setBackgroundColor(0);
        this.webView.setLayoutParams(new RelativeLayout.LayoutParams(-1, -1));
        this.webView.setVisibility(0);
    }

    public void LoadURL(final String str, final boolean z) {
        this.webViewWorkExecutor.execute(() -> this.baseActivity.runOnUiThread(() -> {
            this.allowExternalLinks = z;
            ShowWebView();
            originalURL = str;
            this.webView.loadUrl(str);
        }));
    }

    public void LoadURLPost(final String str, final byte[] bArr, final boolean z) {
        this.webViewWorkExecutor.execute(() -> this.baseActivity.runOnUiThread(() -> {
            this.allowExternalLinks = z;
            ShowWebView();
            originalURL = str;
            this.webView.postUrl(str, bArr);
        }));
    }

    public void SetFrame(final float f, final float f2, final float f3, final float f4) {
        this.webViewWorkExecutor.execute(() -> this.baseActivity.runOnUiThread(() -> {
            RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams((int) f3, (int) f4);
            lp.setMargins((int) f, (int) f2, 0, 0);
            this.webView.setLayoutParams(lp);
        }));
    }

    public void SetBgColor(final int i, final int i2, final int i3, final int i4) {
        this.webViewWorkExecutor.execute(() -> this.baseActivity.runOnUiThread(() ->
            this.webView.setBackgroundColor(Color.argb(i, i2, i3, i4))
        ));
    }

    public void HideWebView() {
        this.webViewWorkExecutor.execute(() -> this.baseActivity.runOnUiThread(() -> {
            WebView wv = this.webView;
            if (wv == null) return;
            wv.stopLoading();
            wv.setVisibility(8);
            wv.loadUrl("about:blank");
            wv.clearHistory();
            DestroyWebView();
        }));
    }

    public void requestPageSource() {
        if (this.webView == null) return;
        this.baseActivity.runOnUiThread(() ->
            this.webView.loadUrl("javascript:NativeApp.pageContent(document.body.innerText)")
        );
    }

    public class WebViewJavascriptInterface {
        WebViewManager webviewManager;

        WebViewJavascriptInterface(WebViewManager wvm) {
            this.webviewManager = wvm;
        }

        @JavascriptInterface
        public void nativeSignIn(String str) {
            Log.d("JavaScriptInterface", "nativeSignIn called! Token: " + str);
            this.webviewManager.nativeOnScriptCall("nativeSignIn", str);
        }

        @JavascriptInterface
        public void onloginselection(String str) {
            Log.d("JavaScriptInterface", "onloginselection called! Token: " + str);
            this.webviewManager.nativeOnScriptCall("onloginselection", str);
        }

        @JavascriptInterface
        public void onnameselection(String str) {
            Log.d("JavaScriptInterface", "onnameselection called! Token: " + str);
            this.webviewManager.nativeOnScriptCall("onnameselection", str);
        }

        @JavascriptInterface
        public void pageContent(String str) {
            Log.d("JavaScriptInterface", "pageContent called! Token: " + str);
            this.webviewManager.nativeOnPageContent(str);
        }

        @JavascriptInterface
        public void openInBrowser(final String str) {
            Log.d("JavaScriptInterface", "openInBrowser called! url: " + str);
            WebViewManager.this.baseActivity.runOnUiThread(() ->
                WebViewManager.this.baseActivity.startActivity(
                    new Intent("android.intent.action.VIEW", Uri.parse(str)))
            );
        }
    }

    private class WebViewClientImpl extends WebViewClient {
        private Activity baseActivity;
        private WebViewCalbackListener webViewCallbacksListener;

        WebViewClientImpl(Activity activity, WebViewCalbackListener listener) {
            this.baseActivity = activity;
            this.webViewCallbacksListener = listener;
        }

        @Override
        public boolean shouldOverrideUrlLoading(WebView webView, String str) {
            Uri uri = Uri.parse(WebViewManager.originalURL);
            Uri uri2 = Uri.parse(str);
            if (!WebViewManager.this.allowExternalLinks || uri.getHost().equals(uri2.getHost())) {
                webView.loadUrl(str);
                return true;
            }
            this.baseActivity.startActivity(new Intent("android.intent.action.VIEW", Uri.parse(str)));
            return true;
        }

        @Override
        public void onPageFinished(WebView webView, String str) {
            WebViewManager.this.webView.loadUrl(
                "javascript:(function f() {var element = document.getElementsByTagName(\"a\");" +
                "for (const value of element) {" +
                "value.addEventListener(\"click\", function(e) {" +
                "  if (e.currentTarget.target == '_blank') {" +
                "    e.preventDefault(); NativeApp.openInBrowser(e.currentTarget.href); return false;" +
                "  }" +
                "});" +
                "}})()");
            this.webViewCallbacksListener.OnPageLoaded(str);
        }

        @Override
        public void onReceivedError(WebView webView, WebResourceRequest request, WebResourceError error) {
            super.onReceivedError(webView, request, error);
            Log.e("WebView", "onReceivedError [" + error.getDescription() + "] : " + request.getUrl());
            this.webViewCallbacksListener.OnError(error.getErrorCode());
        }

        @Override
        public void onReceivedSslError(WebView webView, SslErrorHandler handler, SslError error) {
            super.onReceivedSslError(webView, handler, error);
            Log.e("WebView", "onReceivedSslError [" + error.getPrimaryError() + "] : " + error.toString());
            this.webViewCallbacksListener.OnError(error.getPrimaryError());
        }

        @Override
        public void onReceivedHttpError(WebView webView, WebResourceRequest request, WebResourceResponse response) {
            super.onReceivedHttpError(webView, request, response);
            Log.e("WebView", "onReceivedHttpError [" + response.getStatusCode() + "] : " + request.getUrl());
            this.webViewCallbacksListener.OnError(response.getStatusCode());
        }
    }

    private void clearWebViewDirectories() {
        // getDataDir() is API 24+, and minSdk here is 21. Calling it on an
        // older device raises NoSuchMethodError -- an Error, not an Exception,
        // so the constructor's catch would not have caught it and the executor
        // thread would have taken the process down on startup.
        File dataDir = android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N
            ? this.baseActivity.getDataDir()
            : this.baseActivity.getFilesDir().getParentFile();
        File cacheDir = this.baseActivity.getCacheDir();
        if (dataDir != null) {
            File[] files = dataDir.listFiles();
            if (files != null) {
                for (File f : files) {
                    if (isStaleWebViewDataDirectory(f.getName())) {
                        Log.d("WebViewManager", "Deleting stale WebView data dir: " + f.getAbsolutePath());
                        deleteRecursively(f);
                    }
                }
            }
        }
        if (cacheDir != null) {
            File[] files = cacheDir.listFiles();
            if (files != null) {
                for (File f : files) {
                    if (isStaleWebViewCacheDirectory(f.getName())) {
                        Log.d("WebViewManager", "Deleting stale WebView cache dir: " + f.getAbsolutePath());
                        deleteRecursively(f);
                    }
                }
            }
        }
        safeDeleteDatabase("webview.db");
        safeDeleteDatabase("webviewCache.db");
    }

    private boolean isStaleWebViewDataDirectory(String str) {
        return str.startsWith("app_webview_") && str.matches(".*\\.\\d+$");
    }

    private boolean isStaleWebViewCacheDirectory(String str) {
        return str.startsWith("webview_") && str.matches(".*\\.\\d+$");
    }

    private void safeDeleteDatabase(String str) {
        try {
            Log.d("WebViewManager", "deleteDatabase(" + str + ") = " + this.baseActivity.deleteDatabase(str));
        } catch (Throwable th) {
            Log.e("WebViewManager", "Failed to delete database: " + str, th);
        }
    }

    private boolean deleteRecursively(File file) {
        boolean ok = true;
        if (file != null && file.exists()) {
            if (file.isDirectory()) {
                File[] children = file.listFiles();
                if (children != null) {
                    for (File child : children) {
                        if (!deleteRecursively(child)) ok = false;
                    }
                }
            }
            if (!file.delete()) {
                Log.w("WebViewManager", "Failed to delete: " + file.getAbsolutePath());
                return false;
            }
        }
        return ok;
    }
}
