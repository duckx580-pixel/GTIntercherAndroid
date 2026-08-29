package com.rtsoft.growtopia.databinding;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import androidx.viewbinding.ViewBinding;
import com.rtsoft.growtopia.R;

public final class MainBinding implements ViewBinding {
    public final LinearLayout rootView;
    private final LinearLayout rootView_;

    private MainBinding(LinearLayout root, LinearLayout content) {
        this.rootView_ = root;
        this.rootView = content;
    }

    @Override
    public LinearLayout getRoot() {
        return this.rootView_;
    }

    public static MainBinding inflate(LayoutInflater inflater) {
        return inflate(inflater, null, false);
    }

    public static MainBinding inflate(LayoutInflater inflater, ViewGroup parent, boolean attachToParent) {
        View view = inflater.inflate(R.layout.main, parent, false);
        if (attachToParent) {
            parent.addView(view);
        }
        return bind(view);
    }

    public static MainBinding bind(View view) {
        if (view == null) {
            throw new NullPointerException("rootView");
        }
        LinearLayout ll = (LinearLayout) view;
        return new MainBinding(ll, ll);
    }
}
