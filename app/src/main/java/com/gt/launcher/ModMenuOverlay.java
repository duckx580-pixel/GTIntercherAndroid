package com.gt.launcher;

import android.graphics.Color;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import com.rtsoft.growtopia.SharedActivity;

// A plain Android-View mod menu: a small toggle button plus a command panel,
// added directly into the activity's own view hierarchy
// (SharedActivity.mViewGroup) as ordinary sibling views next to Growtopia's
// own AppGLSurfaceView (mGLView) -- not drawn via OpenGL/ImGui, and not
// hooked into Growtopia's own render call at all.
//
// This replaced an earlier menu that drew itself with ImGui from inside a
// hook on AppRenderer.nativeRender(), sharing Growtopia's own GL thread and
// GL context. An ANR trace from a real test run showed that GL thread
// permanently stuck deep inside libhoudini.so (MEmu's ARM-to-x86 translator)
// with no Java frames and no resolvable symbols at all -- Houdini itself hung
// translating or running something in there and never returned, freezing the
// entire game's rendering, not just the menu. Comparing against
// RealGrowlauncher (a real, working Growtopia launcher with its own mod
// menu) showed its menu is a plain Android view (Jetpack Compose, in their
// case) added on top of the game's layout, never touching OpenGL or the
// game's GL thread at all -- this does the same thing with plain Android
// views. Since none of this ever runs on the GL thread or touches the GL
// context, nothing here can freeze Growtopia's own rendering the way the
// ImGui hook could.
//
// Game interaction goes straight through Growtopia's own already-declared
// public static native methods (SharedActivity.nativeOnKey, etc.) -- the
// same ones Growtopia's own chat box UI calls in AddEditBoxListeners() -- so
// no native hooking is needed here at all. Touch routing needs no special
// handling either: these are ordinary sibling views in the same
// RelativeLayout as mGLView, so Android's own touch dispatch sends a touch
// to whichever view is topmost at that point, and a GONE view (the panel,
// when closed) never receives touches at all.
public final class ModMenuOverlay {
    private static final String[] QUICK_COMMANDS = { "/help", "/who", "/time", "/friends" };
    // android.view.KeyEvent.KEYCODE_ENTER, matching the code Growtopia's own
    // chat box's "Done" button sends via nativeOnKey(1, 13, 13).
    private static final int KEYCODE_ENTER = 13;

    private final SharedActivity mActivity;
    private final View mPanel;
    private final EditText mCommandInput;
    private final TextView mStatus;
    private boolean mOpen;

    public ModMenuOverlay(SharedActivity activity) {
        mActivity = activity;

        View toggleButton = buildToggleButton();
        RelativeLayout.LayoutParams toggleParams = new RelativeLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        toggleParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        toggleParams.addRule(RelativeLayout.ALIGN_PARENT_START);
        toggleParams.setMargins(dp(12), dp(12), 0, 0);
        activity.mViewGroup.addView(toggleButton, toggleParams);

        mPanel = LayoutInflater.from(activity).inflate(R.layout.mod_menu_panel, activity.mViewGroup, false);
        mPanel.setVisibility(View.GONE);
        RelativeLayout.LayoutParams panelParams = new RelativeLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.addRule(RelativeLayout.CENTER_IN_PARENT);
        activity.mViewGroup.addView(mPanel, panelParams);

        mCommandInput = mPanel.findViewById(R.id.mod_menu_command);
        mStatus = mPanel.findViewById(R.id.mod_menu_status);
        Button sendButton = mPanel.findViewById(R.id.mod_menu_send);
        View closeButton = mPanel.findViewById(R.id.mod_menu_close);
        LinearLayout quickRow = mPanel.findViewById(R.id.mod_menu_quick_row);

        toggleButton.setOnClickListener(v -> setOpen(!mOpen));
        closeButton.setOnClickListener(v -> setOpen(false));
        sendButton.setOnClickListener(v -> sendCurrentCommand());
        mCommandInput.setOnEditorActionListener((v, actionId, event) -> {
            sendCurrentCommand();
            return true;
        });

        for (String command : QUICK_COMMANDS) {
            Button button = new Button(activity);
            button.setText(command);
            button.setAllCaps(false);
            button.setTextSize(11f);
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            params.setMarginEnd(dp(4));
            button.setLayoutParams(params);
            button.setOnClickListener(v -> sendCommand(command));
            quickRow.addView(button);
        }
    }

    private View buildToggleButton() {
        TextView button = new TextView(mActivity);
        button.setText("GTL");
        button.setTextColor(Color.WHITE);
        button.setTextSize(12f);
        button.setGravity(Gravity.CENTER);
        button.setBackgroundColor(ContextCompat.getColor(mActivity, R.color.launcher_purple));
        button.setPadding(dp(14), dp(8), dp(14), dp(8));
        return button;
    }

    private void setOpen(boolean open) {
        mOpen = open;
        mPanel.setVisibility(open ? View.VISIBLE : View.GONE);
    }

    private void sendCurrentCommand() {
        String text = mCommandInput.getText().toString();
        if (text.isEmpty()) {
            return;
        }
        sendCommand(text);
        mCommandInput.setText("");
    }

    // Mirrors the exact key-event sequence Growtopia's own chat box uses in
    // SharedActivity.AddEditBoxListeners()/CreateEditBoxBG() (nativeOnKey per
    // typed character, then a keycode-13 "Enter" to submit) rather than going
    // through this app's own EditText -- there is no persistent native-side
    // input buffer we could safely diff against here the way that class's own
    // TextWatcher does with its "m_before" field, so this always sends a
    // fresh string with no backspacing.
    private void sendCommand(String text) {
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            SharedActivity.nativeOnKey(1, 0, c);
            SharedActivity.nativeOnKey(0, 0, c);
        }
        SharedActivity.nativeOnKey(1, KEYCODE_ENTER, KEYCODE_ENTER);
        mStatus.setText("sent: " + text);
    }

    private int dp(int value) {
        float density = mActivity.getResources().getDisplayMetrics().density;
        return Math.round(value * density);
    }
}
