package com.example.videostreamprocessor;

import com.google.common.base.Preconditions;


final class Yuv420 {

    public static void decode(byte[] yuv, int[] rgb, int width, int height) {
        final int size = width * height;

        Preconditions.checkNotNull(yuv, "yuv");
        Preconditions.checkArgument(yuv.length >= size, "buffer 'yuv' size < minimum");
        Preconditions.checkNotNull(rgb, "rgb");
        Preconditions.checkArgument(rgb.length >= size, "Buffer 'rgb' size < minimum");

        int cr = 0;
        int cb = 0;

        for (int j = 0; j < height; j++) {
            int index = j * width;
            final int halfJ = j >> 1;

            for (int i = 0; i < width; i++) {
                int y = yuv[index];

                if (y < 0) {
                    y += 255;
                }

                if ((i & 0x1) != 1) {
                    final int offset = size + halfJ * width + (i >> 1) * 2;

                    cb = yuv[offset];
                    if (cb < 0) {
                        cb += 127;
                    } else {
                        cb -= 128;
                    }

                    cr = yuv[offset + 1];
                    if (cr < 0) {
                        cr += 127;
                    } else {
                        cr -= 128;
                    }
                }

                int r = r(y, cr);
                int g = g(y, cr, cb);
                int b = b(y, cb);

                rgb[index++] = 0xff000000 + (b << 16) + (g << 8) + r;
            }
        }
    }

    private static int limit(int c) {
        return Math.max(0, Math.min(c, 255));
    }

    private static int r(int y, int cr) {
        return limit(y + cr + (cr >> 2) + (cr >> 3) + (cr >> 5));
    }

    private static int g(int y, int cr, int cb) {
        return limit(y - (cb >> 2) + (cb >> 4) + (cb >> 5) - (cr >> 1) + (cr >> 3) + (cr >> 4) + (cr >> 5));
    }

    private static int b(int y, int cb) {
        return limit(y + cb + (cb >> 1) + (cb >> 2) + (cb >> 6));
    }

}
