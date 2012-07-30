package com.example.videostreamprocessor;

import java.io.File;
import java.io.FileOutputStream;

import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.hardware.Camera;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;
import android.support.v4.app.NavUtils;

public class VideoActivity extends Activity implements SurfaceHolder.Callback, Camera.PreviewCallback,
SharedPreferences.OnSharedPreferenceChangeListener {

	//use the JNI/NDK
	static {
		System.loadLibrary("opencv");
	}

	//for the following functions
	public native byte[] getSourceImage();
	public native boolean setSourceImage(int[] data, int w, int h);
	public native boolean doChainOfImageProcessingOperations();
	
	private Camera camera;
	private SurfaceView preview;
	private SurfaceHolder holder;

	private int sizeIndex;
	private Camera.Size size;

	private int[] rgb;
	private final Paint paint = new Paint();

	private boolean skipFrames;
	private int skipBelow;
	private int skipUntil;

	private int frameCount;
	private float oldTime;
	private float newTime;
	
	private float width;
	private float height;
	private float ratio;

	float[] runtimes;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
//		setContentView(R.layout.activity_vision);
		
		final Window window = getWindow();
        window.requestFeature(Window.FEATURE_NO_TITLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_vision);

        final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        preferences.registerOnSharedPreferenceChangeListener(this);

        preview = (SurfaceView) findViewById(R.id.preview);
        
        configure(preferences);
        
        holder = preview.getHolder();
        holder.addCallback(this);
        
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_vision, menu);
		return true;
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		// TODO Auto-generated method stub

	}

    @Override
    protected void onResume() {
        super.onResume();
        if (camera == null) {
            camera = Camera.open();
            final Camera.Parameters parameters = camera.getParameters();
            size = parameters.getSupportedPreviewSizes().get(sizeIndex);
            parameters.setPreviewSize(size.width, size.height);
            camera.setParameters(parameters);
        }
    }

    @Override
    protected void onPause() {
        if (camera != null) {
            camera.setPreviewCallback(null);
            camera.stopPreview();
            camera.release();
            camera = null;
        }
        super.onPause();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (camera != null) {
            preview.setWillNotDraw(false);
            camera.startPreview();

            camera.setPreviewCallback(this);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        this.width = width;
        this.height = height;
        this.ratio = this.width / this.height;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (camera != null) {
            camera.setPreviewCallback(null);
            camera.stopPreview();
            camera.release();
            camera = null;
        }
    }

	@Override
	public void onPreviewFrame(byte[] yuv, Camera camera) {
		if (holder != null) {
			newTime = android.os.SystemClock.uptimeMillis();
			//work out delta time and convert to seconds
			float dt = ((newTime-oldTime)/1000);
		
			float fps = 1.0f / dt;
			
//			Log.d("Captain's Log", "(ms) oldTime-> " + oldTime + " newTime-> " + newTime);
//			Log.d("Captain's Log", "(sec) oldTime-> " + (oldTime/1000) + " newTime-> " + (newTime/1000));
//			
//			Log.d("Captain's Log", "deltaT (ms): "+dt+" @ frame "+ frameCount);
//			Log.d("Captain's Log", "deltaT (sec): "+(dt/1000)+" @ frame "+ frameCount);
//
//			Log.d("Captain's Log", "FpMs: "+fpms);
//			Log.d("Captain's Log", "Fps: "+ fps);
			
			oldTime = newTime;

			if(frameCount > 0 && frameCount < 101){
				runtimes[frameCount-1] = (fps);
			}
			
			if(frameCount == 101){
				String file = "/runtimes.txt";

				//write it to file
				try {

					File root = Environment.getExternalStorageDirectory();

					String localFilePath = root.getPath() + file;



					FileOutputStream fos = new FileOutputStream(localFilePath, false);

					fos.write("Run\tMethod\tFps\n".getBytes());

					for(int i = 0; i < 100; i++){
						fos.write(((i+1)+"\t"+ 2 +"\t"+ runtimes[i] +"\n").getBytes());
					}

					fos.close();
					Log.d("Captain's Log", "Fps were written to runtimes.txt");
				} catch (Exception e) {

					e.printStackTrace();

				}

			}
//            if (skipFrames) {
//                if (frameCount < skipBelow) {
//                    frameCount++;
//                    return;
//                } else if (frameCount >= skipUntil) {
//                    frameCount = 0;
//                    return;
//                }
//            }
//
            frameCount++;

            Canvas canvas = null;

            try {
                canvas = holder.lockCanvas(null);

                if (rgb == null) {
                    rgb = new int[size.width * size.height];
                }

                Yuv420.decode(yuv, rgb, size.width, size.height);

                final float x;
                final float y;

                if (size.width < width && size.height < height) {
                    final float bitmapRatio = (float) size.width / (float) size.height;

                    final float scale;

                    if (bitmapRatio < ratio) {
                        scale = height / (float) size.height;
                    } else {
                        scale = width / (float) size.width;
                    }

                    canvas.scale(scale, scale, canvas.getWidth() / 2f, canvas.getHeight() / 2f);
                    x = width / 2f - size.width / 2f;
                    y = height / 2f - size.height / 2f;
                } else {
                    x = 0;
                    y = 0;
                }

                //process the rgb (in the NDK)
                //get width and height
                int w = size.width;
				int h = size.height;

				//pass the pixels to OpenCV for later processing
				this.setSourceImage(rgb, w, h);
				
				//process sepia toning
				this.doChainOfImageProcessingOperations();
				
				//pull the image back
				byte[] resultData = this.getSourceImage();

				//process the OpenCV returned data back into a usable bitmap and display it
				Bitmap resultPhoto = BitmapFactory.decodeByteArray(resultData, 0, resultData.length);
				
				//set the canvas with the processed bitmap
				canvas.drawBitmap(resultPhoto, 0, 0, paint);
				//end of processing the rgb
				
				
                //canvas.drawBitmap(rgb, 0, size.width, x, y, size.width, size.height, false, paint);
            } finally {
                if (canvas != null) {
                    holder.unlockCanvasAndPost(canvas);
                }
            }
        }

		
	}
	
	private void configure(SharedPreferences preferences) {
//        final String index = preferences.getString("sizeIndex", getString(R.string.defaultSizeIndex));
//
//        if (index != null) {
//            size = null;
//            sizeIndex = Integer.parseInt(index);
//            rgb = null;
//        }

		frameCount = 0;
//        skipFrames = preferences.getBoolean("skipFrames", Boolean.parseBoolean(getString(R.string.defaultSkipFrames)));
        skipFrames = false;
//        final int rate = Integer.parseInt(preferences.getString("skipRate", getString(R.string.defaultSkipRate)));
//        final int divisor = MoreMath.gcd(rate, 100);
//        
        final int rate = 30;
        final int divisor = 10;
        skipBelow = rate / divisor;
        skipUntil = 100 / divisor;
        runtimes = new float [100];
        oldTime = android.os.SystemClock.uptimeMillis();
//        if (preferences.getBoolean("displayName", Boolean.parseBoolean(getString(R.string.defaultDisplayName)))) {
//            name.setVisibility(View.VISIBLE);
//        } else {
//            name.setVisibility(View.INVISIBLE);
//        }
    }



}
