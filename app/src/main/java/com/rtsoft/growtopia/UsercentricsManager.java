package com.rtsoft.growtopia;

import android.app.Activity;
import android.util.Log;

import com.usercentrics.sdk.UsercentricsConsentHistoryEntry;
import com.usercentrics.sdk.UsercentricsServiceConsent;
import com.usercentrics.sdk.models.settings.UsercentricsConsentType;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

// The 5.55 engine calls InitWithRuleSet/InitWithSettings during startup as
// part of its Usercentrics (GDPR consent) integration, then blocks waiting
// for the InitFinish native callback before it renders its first frame at
// all. The original stub versions of these methods were no-ops that never
// called InitFinish, so the engine ran with no crash but produced a
// permanent black screen -- confirmed against a sibling project
// (duckx580-pixel/Growlauncher, branch claude/launcher-crash-on-open-3id13g)
// that hit and diagnosed the exact same symptom independently, with a commit
// literally titled "Fix black screen: call InitFinish(true) after
// Usercentrics init stubs".
public class UsercentricsManager {
    private static final String TAG = "UsercentricsManager";

    private Activity baseContext;

    public UsercentricsManager(Activity activity) {
        this.baseContext = activity;
    }

    // Native callbacks -- called from the UI thread (matches real 5.55 implementation)
    public native void InitFinish(boolean success);
    public native void OnConsentFetchedFail(int code, String message);
    public native void OnConsentFetchedSuccess(List<UsercentricsServiceConsent> list);

    private List<UsercentricsServiceConsent> buildAcceptedConsentList() {
        UsercentricsConsentHistoryEntry historyEntry = new UsercentricsConsentHistoryEntry(
                true, UsercentricsConsentType.EXPLICIT, System.currentTimeMillis());
        List<UsercentricsConsentHistoryEntry> history = new ArrayList<>();
        history.add(historyEntry);

        UsercentricsServiceConsent consent = new UsercentricsServiceConsent(
                "growtopia",   // templateId
                true,          // status = accepted
                history,
                UsercentricsConsentType.EXPLICIT,
                "Ubisoft",     // dataProcessor
                "1.0",         // version
                true,          // isEssential
                "Essential"    // category
        );
        return Collections.singletonList(consent);
    }

    public void InitWithRuleSet(String str) {
        Log.d(TAG, "InitWithRuleSet called, ruleSetId=" + str);
        baseContext.runOnUiThread(() -> {
            Log.d(TAG, "InitWithRuleSet -> calling InitFinish(true)");
            try { InitFinish(true); } catch (UnsatisfiedLinkError e) {
                Log.w(TAG, "InitFinish unavailable: " + e.getMessage());
            }
        });
    }

    public void InitWithSettings(String str) {
        Log.d(TAG, "InitWithSettings called, settingsId=" + str);
        baseContext.runOnUiThread(() -> {
            Log.d(TAG, "InitWithSettings -> calling InitFinish(true)");
            try { InitFinish(true); } catch (UnsatisfiedLinkError e) {
                Log.w(TAG, "InitFinish unavailable: " + e.getMessage());
            }
        });
    }

    public void CheckConsentState() {
        Log.d(TAG, "CheckConsentState called");
        // Call directly on the calling thread, matching FetchUserConsent's synchronous pattern.
        // Using runOnUiThread here could deadlock: the GL thread calls CheckConsentState while
        // holding internal engine state, the callback gets posted to the UI thread, and if the
        // UI thread is blocked in nativeOnTouch waiting for the GL thread, neither can proceed.
        try { OnConsentFetchedSuccess(buildAcceptedConsentList()); } catch (UnsatisfiedLinkError e) {
            Log.w(TAG, "OnConsentFetchedSuccess unavailable: " + e.getMessage());
        }
    }

    public void FetchUserConsent(List<UsercentricsServiceConsent> list) {
        Log.d(TAG, "FetchUserConsent called, list=" + (list == null ? "null" : "size=" + list.size()));
        List<UsercentricsServiceConsent> consents =
                (list != null && !list.isEmpty()) ? list : buildAcceptedConsentList();
        Log.d(TAG, "FetchUserConsent -> calling OnConsentFetchedSuccess with " + consents.size() + " entries");
        try { OnConsentFetchedSuccess(consents); } catch (UnsatisfiedLinkError e) {
            Log.w(TAG, "OnConsentFetchedSuccess unavailable: " + e.getMessage());
        }
    }

    public void RequestConsentSettings() {
        Log.d(TAG, "RequestConsentSettings called -> delegating to CheckConsentState");
        CheckConsentState();
    }

    public void ShowConsentSettings() {
        Log.d(TAG, "ShowConsentSettings called -> delegating to CheckConsentState");
        CheckConsentState();
    }
}
