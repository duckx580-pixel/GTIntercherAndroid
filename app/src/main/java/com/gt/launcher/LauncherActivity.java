package com.gt.launcher;

import android.animation.ObjectAnimator;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.OvershootInterpolator;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class LauncherActivity extends AppCompatActivity {

    private LightningView lightningView;
    private boolean launching = false;
    private static final int OVERLAY_REQ = 1001;

    private volatile boolean preparing = false;
    private ViewGroup preparingOverlay;
    private TextView preparingText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);

        lightningView = findViewById(R.id.lightning_view);

        // Entrance animation: fade in + slide up from bottom
        View root = getWindow().getDecorView();
        root.setAlpha(0f);
        root.animate().alpha(1f).setDuration(500).setInterpolator(new DecelerateInterpolator()).start();

        // Animate the cards with staggered entrance
        animateCardEntrance();

        // LAUNCH card
        View cardLaunch = findViewById(R.id.card_launch);
        cardLaunch.setOnClickListener(v -> onLaunchClicked());

        // Guest mode
        View btnGuest = findViewById(R.id.btn_guest_mode);
        btnGuest.setOnClickListener(v -> {
            if (launching) return;
            if (!GameSetup.isDeviceSupported()) {
                Toast.makeText(this,
                    "This device is 32-bit only. Growtopia 5.55 requires a 64-bit (arm64) device.",
                    Toast.LENGTH_LONG).show();
                return;
            }
            if (!GameSetup.isGrowtopiaInstalled(this)) {
                Toast.makeText(this,
                    "Growtopia is not installed. Install it first, then press Launch.",
                    Toast.LENGTH_LONG).show();
                return;
            }

            Toast.makeText(this, "Guest mode: Launching without overlay...", Toast.LENGTH_SHORT).show();
            launching = true;

            // Guest mode skips the overlay permission, not the game files: it
            // still needs the native libraries unpacked before starting.
            if (GameSetup.needsExtraction(this)) {
                prepareGameFiles();
            } else {
                startGrowtopiaActivity();
            }
        });

        // Switch version
        View btnSwitch = findViewById(R.id.btn_switch_version);
        btnSwitch.setOnClickListener(v ->
            Toast.makeText(this, "Version switching coming soon!", Toast.LENGTH_SHORT).show()
        );

        // Placeholder cards
        setToast(R.id.card_script_hub, "Script Hub coming soon!");
        setToast(R.id.card_settings, "Settings coming soon!");
        setToast(R.id.card_lua, "Lua Manager coming soon!");
        setToast(R.id.card_sound, "Audio tools coming soon!");
        setToast(R.id.card_theme, "Theme editor coming soon!");

        // Status pulse animation
        View dot = findViewById(R.id.dot_status);
        ObjectAnimator pulse = ObjectAnimator.ofFloat(dot, "alpha", 1f, 0.3f, 1f);
        pulse.setDuration(1800);
        pulse.setRepeatCount(ObjectAnimator.INFINITE);
        pulse.start();
    }

    private void animateCardEntrance() {
        int[] ids = {
            R.id.card_launch, R.id.card_script_hub, R.id.card_settings,
            R.id.card_lua, R.id.card_sound, R.id.card_theme
        };
        for (int i = 0; i < ids.length; i++) {
            View card = findViewById(ids[i]);
            if (card == null) continue;
            card.setTranslationY(60f);
            card.setAlpha(0f);
            card.animate()
                .translationY(0f)
                .alpha(1f)
                .setStartDelay(200 + i * 60L)
                .setDuration(350)
                .setInterpolator(new OvershootInterpolator(0.8f))
                .start();
        }
    }

    private void setToast(int id, String msg) {
        View v = findViewById(id);
        if (v != null) {
            v.setOnClickListener(view ->
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            );
        }
    }

    private void onLaunchClicked() {
        if (launching) return;

        if (!GameSetup.isDeviceSupported()) {
            Toast.makeText(this,
                "This device is 32-bit only. Growtopia 5.55 requires a 64-bit (arm64) device.",
                Toast.LENGTH_LONG).show();
            return;
        }

        if (!GameSetup.isGrowtopiaInstalled(this)) {
            Toast.makeText(this,
                "Growtopia is not installed. Install it first, then press Launch.",
                Toast.LENGTH_LONG).show();
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            Toast.makeText(this,
                "Overlay permission required for mod menu. Grant it and press Launch again.",
                Toast.LENGTH_LONG).show();
            startActivityForResult(
                new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())),
                OVERLAY_REQ
            );
            return;
        }

        launching = true;

        // Growtopia's libraries may still be packed inside its APK. Unpacking
        // is real disk I/O, so it happens here, off the main thread and behind
        // a progress overlay, rather than inside the game activity where it
        // used to block startup and trigger an ANR.
        if (GameSetup.needsExtraction(this)) {
            prepareGameFiles();
        } else {
            animateLaunch();
        }
    }

    private void prepareGameFiles() {
        preparing = true;
        showPreparing("Preparing game files...");

        new Thread(() -> {
            final boolean ok = GameSetup.extract(
                getApplicationContext(),
                message -> runOnUiThread(() -> updatePreparing(message))
            );

            runOnUiThread(() -> {
                preparing = false;
                if (isFinishing() || isDestroyed()) return;

                hidePreparing();
                if (ok) {
                    animateLaunch();
                } else {
                    launching = false;
                    Toast.makeText(this,
                        "Could not read Growtopia's game files. Try Launch again.",
                        Toast.LENGTH_LONG).show();
                }
            });
        }, "gtl-extract").start();
    }

    private void showPreparing(String message) {
        if (preparingOverlay == null) {
            LinearLayout box = new LinearLayout(this);
            box.setOrientation(LinearLayout.VERTICAL);
            box.setGravity(Gravity.CENTER);
            box.setBackgroundColor(0xE6000000);
            box.setClickable(true); // swallow taps while we work

            ProgressBar spinner = new ProgressBar(this);
            box.addView(spinner);

            preparingText = new TextView(this);
            preparingText.setTextColor(0xFFFFFFFF);
            preparingText.setGravity(Gravity.CENTER);
            preparingText.setPadding(48, 32, 48, 0);
            box.addView(preparingText);

            preparingOverlay = box;
            addContentView(preparingOverlay, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }

        preparingOverlay.setVisibility(View.VISIBLE);
        updatePreparing(message);
    }

    private void updatePreparing(String message) {
        if (preparingText != null) {
            preparingText.setText(message);
        }
    }

    private void hidePreparing() {
        if (preparingOverlay != null) {
            preparingOverlay.setVisibility(View.GONE);
        }
    }

    private void animateLaunch() {
        View root = getWindow().getDecorView();

        // Flash effect: quick brightness surge
        View cardLaunch = findViewById(R.id.card_launch);
        if (cardLaunch != null) {
            cardLaunch.animate().scaleX(0.95f).scaleY(0.95f).setDuration(100)
                .withEndAction(() -> cardLaunch.animate().scaleX(1.05f).scaleY(1.05f).setDuration(80)
                    .withEndAction(() -> {
                        // Fade out whole screen, then launch
                        root.animate()
                            .alpha(0f)
                            .setDuration(350)
                            .setInterpolator(new AccelerateDecelerateInterpolator())
                            .withEndAction(this::startGrowtopiaActivity)
                            .start();
                    }).start()
                ).start();
        } else {
            root.animate()
                .alpha(0f)
                .setDuration(350)
                .withEndAction(this::startGrowtopiaActivity)
                .start();
        }
    }

    private void startGrowtopiaActivity() {
        Intent intent = new Intent(this, Main.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OVERLAY_REQ) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Settings.canDrawOverlays(this)) {
                Toast.makeText(this, "Permission granted! Tap Launch.", Toast.LENGTH_SHORT).show();
            }
            launching = false;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Leave the flag alone while extraction is still running in the
        // background, or coming back to the launcher would let a second
        // extraction start on top of the first.
        if (!preparing) {
            launching = false;
        }
        View root = getWindow().getDecorView();
        if (root.getAlpha() < 1f) {
            root.animate().alpha(1f).setDuration(300).start();
        }
        if (lightningView != null) lightningView.startAnimation();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (lightningView != null) lightningView.stopAnimation();
    }
}
