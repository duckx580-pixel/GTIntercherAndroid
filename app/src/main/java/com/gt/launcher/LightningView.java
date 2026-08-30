package com.gt.launcher;

import android.content.Context;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class LightningView extends View {

    private final Paint boltPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint boltGlowPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint particlePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint glowBgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final Random rng = new Random();
    private final List<LightningBolt> bolts = new ArrayList<>();
    private final List<Particle> particles = new ArrayList<>();

    private boolean running = false;
    private long lastBoltTime = 0;

    private static final int[] PARTICLE_COLORS = {
        0xFF7C3AED, 0xFFA78BFA, 0xFF8B5CF6, 0xFF6D28D9, 0xFF3B82F6, 0xFFFFFFFF
    };

    public LightningView(Context context) {
        super(context);
        initPaints();
    }

    public LightningView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initPaints();
    }

    private void initPaints() {
        boltPaint.setStyle(Paint.Style.STROKE);
        boltPaint.setStrokeWidth(1.8f);
        boltPaint.setColor(0xFFA78BFA);
        boltPaint.setStrokeCap(Paint.Cap.ROUND);

        boltGlowPaint.setStyle(Paint.Style.STROKE);
        boltGlowPaint.setStrokeWidth(7f);
        boltGlowPaint.setColor(0x557C3AED);
        boltGlowPaint.setStrokeCap(Paint.Cap.ROUND);
        boltGlowPaint.setMaskFilter(new BlurMaskFilter(10, BlurMaskFilter.Blur.NORMAL));

        particlePaint.setStyle(Paint.Style.FILL);

        glowBgPaint.setStyle(Paint.Style.FILL);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        startAnimation();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        stopAnimation();
    }

    public void startAnimation() {
        running = true;
        postDelayed(ticker, 16);
    }

    public void stopAnimation() {
        running = false;
        removeCallbacks(ticker);
    }

    private final Runnable ticker = new Runnable() {
        @Override
        public void run() {
            if (!running || getWidth() == 0) {
                if (running) postDelayed(this, 32);
                return;
            }

            long now = System.currentTimeMillis();

            // Spawn a bolt every ~600–1200ms
            if (now - lastBoltTime > 600 + rng.nextInt(600) && bolts.size() < 5) {
                spawnBolt();
                lastBoltTime = now;
            }

            // Spawn floating particles randomly
            if (rng.nextFloat() < 0.4f && particles.size() < 40) {
                spawnParticle();
            }

            // Update bolts
            for (int i = bolts.size() - 1; i >= 0; i--) {
                LightningBolt b = bolts.get(i);
                b.alpha -= 0.04f;
                if (b.alpha <= 0f) bolts.remove(i);
            }

            // Update particles
            for (int i = particles.size() - 1; i >= 0; i--) {
                Particle p = particles.get(i);
                p.x += p.vx;
                p.y += p.vy;
                p.alpha -= 0.012f;
                p.vy -= 0.015f; // slight upward drift
                if (p.alpha <= 0f) particles.remove(i);
            }

            invalidate();
            postDelayed(this, 16);
        }
    };

    private void spawnBolt() {
        float startX = rng.nextFloat() * getWidth();
        bolts.add(new LightningBolt(startX, 0, getHeight(), rng));
    }

    private void spawnParticle() {
        float x = rng.nextFloat() * getWidth();
        float y = rng.nextFloat() * getHeight();
        Particle p = new Particle();
        p.x = x;
        p.y = y;
        p.vx = (rng.nextFloat() - 0.5f) * 1.5f;
        p.vy = -(0.3f + rng.nextFloat() * 1.5f);
        p.alpha = 0.4f + rng.nextFloat() * 0.6f;
        p.size = 1f + rng.nextFloat() * 2.5f;
        p.color = PARTICLE_COLORS[rng.nextInt(PARTICLE_COLORS.length)];
        particles.add(p);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        // Radial glow at center-top
        if (getWidth() > 0 && getHeight() > 0) {
            RadialGradient grad = new RadialGradient(
                getWidth() / 2f, 0,
                getWidth() * 0.7f,
                new int[]{0x257C3AED, 0x00000000},
                new float[]{0f, 1f},
                Shader.TileMode.CLAMP
            );
            glowBgPaint.setShader(grad);
            canvas.drawRect(0, 0, getWidth(), getHeight(), glowBgPaint);
        }

        // Draw particles
        for (Particle p : particles) {
            int alpha = (int) (p.alpha * 255);
            particlePaint.setColor((p.color & 0x00FFFFFF) | (alpha << 24));
            canvas.drawCircle(p.x, p.y, p.size, particlePaint);
        }

        // Draw lightning bolts (glow first, then crisp line)
        for (LightningBolt bolt : bolts) {
            int glowAlpha = (int) (bolt.alpha * 80);
            boltGlowPaint.setAlpha(glowAlpha);
            canvas.drawPath(bolt.path, boltGlowPaint);

            int lineAlpha = (int) (bolt.alpha * 255);
            boltPaint.setAlpha(lineAlpha);
            canvas.drawPath(bolt.path, boltPaint);

            // Draw branch if present
            if (bolt.branchPath != null) {
                boltGlowPaint.setAlpha(glowAlpha / 2);
                canvas.drawPath(bolt.branchPath, boltGlowPaint);
                boltPaint.setAlpha(lineAlpha / 2);
                canvas.drawPath(bolt.branchPath, boltPaint);
            }
        }
    }

    // ── Inner classes ──────────────────────────────────────────────

    private static class LightningBolt {
        final Path path;
        Path branchPath;
        float alpha;

        LightningBolt(float startX, float startY, int viewHeight, Random rng) {
            path = new Path();
            path.moveTo(startX, startY);

            float x = startX;
            float y = startY;
            float targetY = viewHeight * (0.35f + rng.nextFloat() * 0.55f);
            boolean branched = false;

            while (y < targetY) {
                float stepY = 12f + rng.nextFloat() * 18f;
                float stepX = (rng.nextFloat() - 0.5f) * 50f;
                x = Math.max(0, Math.min(x + stepX, 1000));
                y = Math.min(y + stepY, targetY);
                path.lineTo(x, y);

                // Maybe create a small branch
                if (!branched && rng.nextFloat() < 0.3f && y < targetY * 0.7f) {
                    branched = true;
                    branchPath = new Path();
                    branchPath.moveTo(x, y);
                    float bx = x, by = y;
                    float branchTarget = by + 40 + rng.nextFloat() * 40;
                    while (by < branchTarget) {
                        float bsy = 10f + rng.nextFloat() * 12f;
                        float bsx = (rng.nextFloat() - 0.5f) * 30f;
                        bx += bsx;
                        by = Math.min(by + bsy, branchTarget);
                        branchPath.lineTo(bx, by);
                    }
                }
            }

            alpha = 0.7f + rng.nextFloat() * 0.3f;
        }
    }

    private static class Particle {
        float x, y, vx, vy, alpha, size;
        int color;
    }
}
