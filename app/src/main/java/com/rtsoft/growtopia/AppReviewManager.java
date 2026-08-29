package com.rtsoft.growtopia;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.play.core.review.ReviewException;
import com.google.android.play.core.review.ReviewInfo;
import com.google.android.play.core.review.ReviewManager;
import com.google.android.play.core.review.ReviewManagerFactory;

public class AppReviewManager {
    private Context baseContext;
    private ReviewManager manager;

    static void lambda$RequestReviewFlow$0(Task task) {
    }

    public AppReviewManager(Context context) {
        this.baseContext = context;
    }

    public void OnCreate() {
        this.manager = ReviewManagerFactory.create(this.baseContext);
    }

    public void RequestReviewFlow() {
        this.manager.requestReviewFlow().addOnCompleteListener(new OnCompleteListener<ReviewInfo>() {
            @Override
            public void onComplete(Task<ReviewInfo> task) {
                AppReviewManager.this.lambda$RequestReviewFlow$1(task);
            }
        });
    }

    private void lambda$RequestReviewFlow$1(Task<ReviewInfo> task) {
        if (task.isSuccessful()) {
            this.manager.launchReviewFlow((Activity) this.baseContext, task.getResult()).addOnCompleteListener(new OnCompleteListener<Void>() {
                @Override
                public void onComplete(Task<Void> task2) {
                    AppReviewManager.lambda$RequestReviewFlow$0(task2);
                }
            });
        } else {
            try {
                Log.e(((Activity) this.baseContext).getPackageName(), "[APP_REVIEW] error: " + ((ReviewException) task.getException()).getErrorCode());
            } catch (Exception e) {
                Log.e("AppReviewManager", "Error in review flow", e);
            }
        }
    }
}
