//
//  ImageProcessor.cpp
//  FaceIt
//
//  Created by Beau Johnston on 1/06/12.
//  Copyright (c) 2012 OpenParallel.com all rights reserved.
//

#include "ImageProcessor.h"

/*
 * Private variable to track the number of features
 */
int numberOfFeatures = 0;
char* pwd = (char*)"";
#ifdef ANDROID
//this is only used as a flag for the android device to wait for the NDK
bool processingFinished = false;
#endif


/*
 * Private Utility functions
 */

char* stringCat(char*s, char*s1){
    char* target = new char[strlen(s) + strlen(s1) + 1];
    strcpy(target, s);
    strcat(target, s1);
    return target;
}

void Log(char* message, bool errorFlag){
    //append a newline to the end of the log string
    message = stringCat(message, (char*)"\n");
    
#ifdef ANDROID
    //android log
    if (errorFlag) {
        LOGE(message);
    }
    else{
        LOGV(message);
    }
#endif
    
#ifndef ANDROID
    //regular log
    if (errorFlag) {
        printf("%s", message);
        exit(EXIT_FAILURE);
    }
    else {
        printf("%s", message);
    }
#endif
    return;
}


void applySepiaTone(IplImage* target){
    for (int ix=0; ix<target->width; ix++) {
        for (int iy=0; iy<target->height; iy++) {
            
            //extract each pixel
            int r = cvGet2D(target, iy, ix).val[2];
            int g = cvGet2D(target, iy, ix).val[1];
            int b = cvGet2D(target, iy, ix).val[0];
            
            //generate a grayscale pixel
            int p = round(((r+g+b)/3));
            
            //to generate sepia tone colouration, use the colourspace
            //rgb (+40,+20,-20)
            //CvScalar expects bgr colour so:
            CvScalar bgr = cvScalar(p-20, p+20, p+40);
            
            cvSet2D(target, iy, ix, bgr);
        }
    }
}

void overlayImage(IplImage* target, IplImage* source, int x, int y) {
    
    for (int ix=0; ix<source->width; ix++) {
        for (int iy=0; iy<source->height; iy++) {
            int r = cvGet2D(source, iy, ix).val[2];
            int g = cvGet2D(source, iy, ix).val[1];
            int b = cvGet2D(source, iy, ix).val[0];
            CvScalar bgr = cvScalar(b, g, r);
            cvSet2D(target, iy+y, ix+x, bgr);
        }
    }
}


/*
 * End of Private utility functions
 */








/*
 * Public feature detection functions
 */


IplImage* drawRectangleOnImage(CvRect featureRect, IplImage*inputImage){
    
    cvRectangle(inputImage, cvPoint(featureRect.x, featureRect.y), cvPoint(featureRect.x + featureRect.width, featureRect.y + featureRect.height), cvScalar(0, 255, 255, 255), 3, 1, 0);
    
    return inputImage;
}

IplImage* drawRectangleOnImageWithColour(CvRect featureRect, IplImage*inputImage,CvScalar colour){
    
    cvRectangle(inputImage, cvPoint(featureRect.x, featureRect.y), cvPoint(featureRect.x + featureRect.width, featureRect.y + featureRect.height), colour, 3, 1, 0);
    
    return inputImage;
}

IplImage* drawRectangleOnImageWithColourAndFilled(CvRect featureRect, IplImage*inputImage,CvScalar colour){
    
    cvRectangle(inputImage, cvPoint(featureRect.x, featureRect.y), cvPoint(featureRect.x + featureRect.width, featureRect.y + featureRect.height), colour, CV_FILLED, 1, 0);
    
    return inputImage;
}

/*
 * End of public feature detection functions
 */

#ifndef ANDROID
void setWorkingDir(char* wd){
    pwd = wd;
}
#endif

/*
 * Now for android stuff
 */
#ifdef ANDROID
JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_doChainOfImageProcessingOperations(JNIEnv* env,
                                                                                        jobject thiz){
    processingFinished = false;
    
    applySepiaTone(m_sourceImage);
    
    processingFinished = true;
    return true;
    
}


JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_applyFunhouseEffect(JNIEnv* env,
                                                                            jobject thiz){
    
    processingFinished = false;
    
    bool erode = true;
	bool circle = false;
	bool dilate = true;
	bool mirror = true;
    
    
    // Get one frame
    IplImage* frame = cvCloneImage(m_sourceImage);
    cvReleaseImage(&m_sourceImage);
        
    if( !frame ) {
        return true;
    }
    
    if(mirror) {
        int halfFrame = frame->width/2;
        int frameBytes = frame->width*3-1;
        for(int i = 0; i < frame->height; i++) {
            int offset = i*frame->width*3;
            for(int j = 0; j < halfFrame; j++) {
                int jBytes = offset+frameBytes-(j*3);
                int ojBytes = offset+(j*3);
                frame->imageData[jBytes-2] = frame->imageData[ojBytes];
                frame->imageData[jBytes-1] = frame->imageData[ojBytes+1];
                frame->imageData[jBytes] = frame->imageData[ojBytes+2];
            }
        }
    }
        
    if(erode)
        cvErode(frame,frame,0,2);
    if(circle)
        cvCircle(frame, cvPoint(100,100), 20, cvScalar(0,255,0), 1);
    if(dilate)
        cvDilate(frame,frame);
        
    m_sourceImage = cvCloneImage(frame);
    
    cvReleaseImage(&frame);
    
	processingFinished = true;
    
    return processingFinished;
}


JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_applySketchbookEffect(JNIEnv* env,
                                                                                         jobject thiz){
    
    processingFinished = false;
    
    int col_1, row_1;
    uchar b_1, g_1, r_1, b_2, g_2, r_2, b_d, g_d, r_d;

    IplImage* img = cvCloneImage(m_sourceImage);
    cvReleaseImage( &m_sourceImage );
    
    IplImage* img1 = cvCreateImage( cvSize( img->width,img->height ), img->depth, img->nChannels);
    IplImage* img2 = cvCreateImage( cvSize( img->width,img->height ), img->depth, img->nChannels);
    IplImage* dst = cvCreateImage( cvSize( img->width,img->height ), img->depth, img->nChannels);
    IplImage* gray= cvCreateImage(cvGetSize(img), img->depth, 1);

    cvNot(img, img1);
    //   cvSmooth(img1, img2, CV_BLUR, 25,25,0,0);
    cvSmooth(img, img2, CV_GAUSSIAN, 7, 7, 0, 0); // last fix :)

    for( row_1 = 0; row_1 < img1->height; row_1++ )
    {
        for ( col_1 = 0; col_1 < img1->width; col_1++ )
        {
            b_1 = CV_IMAGE_ELEM( img1, uchar, row_1, col_1 * 3 );
            g_1 = CV_IMAGE_ELEM( img1, uchar, row_1, col_1 * 3 + 1 );
            r_1 = CV_IMAGE_ELEM( img1, uchar, row_1, col_1 * 3 + 2 );
            
            b_2 = CV_IMAGE_ELEM( img2, uchar, row_1, col_1 * 3 );
            g_2 = CV_IMAGE_ELEM( img2, uchar, row_1, col_1 * 3 + 1 );
            r_2 = CV_IMAGE_ELEM( img2, uchar, row_1, col_1 * 3 + 2 );
            
            //            b_d = b_1 + b_2;
            //            g_d = g_1 + g_2;
            //            r_d = r_1 + r_2;
            
//            b_d = min(255, b_1 + b_2);
//            g_d = min(255, g_1 + g_2);
//            r_d = min(255, r_1 + r_2);
//            
            if (b_1+b_2 < 255) {
                b_d = 255;
            }else{
                b_d = b_1+b_2;
            }
            
            if (g_1+g_2 < 255) {
                g_d = 255;
            }else{
                g_d = g_1+g_2;
            }
            
            if (r_1+r_2 < 255) {
                r_d = 255;
            }else{
                r_d = r_1+r_2;
            }
            
            
            dst->imageData[img1->widthStep * row_1 + col_1* 3] = b_d;
            dst->imageData[img1->widthStep * row_1 + col_1 * 3 + 1] = g_d;
            dst->imageData[img1->widthStep * row_1 + col_1 * 3 + 2] = r_d;
        }
    }
    cvCvtColor(dst, gray, CV_BGR2GRAY);

    m_sourceImage = cvCloneImage(gray);
    
    cvReleaseImage( &img );
    cvReleaseImage( &img1 ); // Yes, you must release all the allocated memory.
    cvReleaseImage( &img2 );
    cvReleaseImage( &dst );
    cvReleaseImage( &gray);
    
    processingFinished = true;
    
    return processingFinished;
}


JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_applyNeonising(JNIEnv* env,
                                                                            jobject thiz){
    
    processingFinished = false;
    
    
    IplImage* sourceGrey = cvCreateImage(cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1);
    IplImage* sobelised = cvCreateImage(cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1);
    IplImage* threshed = cvCreateImage(cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1);
    IplImage* equalised = cvCreateImage(cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1);
    
    cvCvtColor(m_sourceImage, sourceGrey, CV_BGR2GRAY);
    
    cvReleaseImage(&m_sourceImage);
//    //get grayscale
//    IplImage* r = cvCreateImage( cvGetSize(source), IPL_DEPTH_8U, 1 ); 
//    IplImage* g = cvCreateImage( cvGetSize(source), IPL_DEPTH_8U, 1 ); 
//    IplImage* b = cvCreateImage( cvGetSize(source), IPL_DEPTH_8U, 1 );
//    
//    
//    // Split image onto the color planes. 
//    cvSplit( source, b, g, r, NULL );
//    
//    cvAddWeighted( r, 1./3., g, 1./3., 0.0, sourceGrey ); 
//    cvAddWeighted( sourceGrey, 2./3., b, 1./3., 0.0, sourceGrey );
//    
//    cvReleaseImage(&r);
//    cvReleaseImage(&b);
//    cvReleaseImage(&g);
    
    

    //equalise histogram
    cvEqualizeHist(sourceGrey, equalised);
    
    double minVal, maxVal;
    cvMinMaxLoc(equalised, &minVal, &maxVal, NULL, NULL, NULL); //find minimum and maximum intensities
    
    m_sourceImage = cvCreateImage(cvGetSize(sourceGrey), 8, 3);
    cvMerge(sourceGrey, sourceGrey, sourceGrey, NULL, m_sourceImage);
    
#ifdef EighteesStyle
    int stepSize = 0;
    //int stepSize = 2;
    
    for (int j=0; j < 10; j+=stepSize) {
    
        cvThreshold(equalised, threshed, (maxVal*(j*0.1)), (maxVal*((j+stepSize)*0.1)), CV_THRESH_BINARY);
            
        //apply neon ;)
        CvMemStorage* contour_storage = cvCreateMemStorage(0);
        CvSeq* contours = NULL;
        
        int numContours = cvFindContours(threshed, contour_storage, &contours, sizeof(CvContour),
                                         CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0));
        
        //if no contours were found return early
        if(numContours  < 1){
            processingFinished = true;
            
            return processingFinished;
        }
        
        //list of Neon Light Colours 
        //
        //rgb       %   %   %
        //---------------------
        //turquise  0   255 255
        //lemon     255 255 0
        //spring    0   255 0
        //magenta   255 0   255
        //lime      128 255 0
        //tangerine 255 128 0
        
        
        
        int insChoice = rand()%5;
        int outsChoice = rand()%5;
        
        CvScalar ins;
        CvScalar outs;
        
        switch (insChoice) {
            case 0:
                ins = CV_RGB(0,255,255);
                break;
            case 1:
                ins = CV_RGB(255,255,0);
                break;
            case 2:
                ins = CV_RGB(0,255,0);
                break;
            case 3:
                ins = CV_RGB(255,0,255);
                break;
            case 4:
                ins = CV_RGB(128,255,0);
                break;
            case 5:
                ins = CV_RGB(255,128,0);
                break;
            default:
                break;
        }
        
        switch (outsChoice) {
            case 0:
                outs = CV_RGB(0,255,255);
                break;
            case 1:
                outs = CV_RGB(255,255,0);
                break;
            case 2:
                outs = CV_RGB(0,255,0);
                break;
            case 3:
                outs = CV_RGB(255,0,255);
                break;
            case 4:
                outs = CV_RGB(128,255,0);
                break;
            case 5:
                outs = CV_RGB(255,128,0);
                break;
            default:
                break;
        }
        
        for( CvSeq* c = contours; c != NULL; c = c->h_next ){
            cvDrawContours(m_sourceImage, c, ins, outs, 1, 25, 8);
        }
        
        cvReleaseMemStorage(&contour_storage);
        
    }
    
#else
    
    
     
     //LOGE("Maxval is -> %f", maxVal);
     
     //now we have time for binary thesholding
     //cvAdaptiveThreshold(equalised,threshed,maxVal,CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_TRUNC, 50);
     
     //get the top 40%
     cvThreshold(equalised, threshed, (maxVal*0.6), (maxVal*1.0), CV_THRESH_BINARY);
     
     //perform an Closing morphology operation
     IplConvKernel* ellipse = cvCreateStructuringElementEx(5,5,0,0,CV_SHAPE_ELLIPSE,NULL);
     
     
     cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     
     //    cvErode(threshed,threshed, ellipse,1);
     //    cvDilate(threshed,threshed, ellipse,1);
     
     for (int i = 0; i < 3; i++) {
         cvSmooth(threshed, threshed, CV_GAUSSIAN,3,3);
         cvThreshold(threshed, threshed, 1, 255, CV_THRESH_BINARY);
     }
     
     //    
     //    cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     //    cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     //    cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 1);
     //    
     
     //cvDilate(threshed,threshed, ellipse,5);
     
     //cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_OPEN, 4);
     //    int numberOfMorphs = 4;
     
     //    for (int i = 0; i < numberOfMorphs; i++) {
     //for every 1 closing operation do 2 iterations of opening (really remove all fine features)
     //        cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_CLOSE, 4);
     
     //cvMorphologyEx(threshed, threshed, NULL, ellipse, CV_MOP_OPEN, 1);
     //    }
     
    
    //    m_sourceImage = cvCloneImage(threshed);
    //    processingFinished = true;
    //    return processingFinished;
    //    /*
    
    
    //apply neon ;)
    CvMemStorage* contour_storage = cvCreateMemStorage(0);
    CvSeq* contours = NULL;
    
    int numContours = cvFindContours(threshed, contour_storage, &contours, sizeof(CvContour),
                                     CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0));
    
    //if no contours were found return early
    if(numContours  < 1){
        processingFinished = true;
        
        return processingFinished;
    }
    
    //list of Neon Light Colours 
    //
    //rgb       %   %   %
    //---------------------
    //turquise  0   255 255
    //lemon     255 255 0
    //spring    0   255 0
    //magenta   255 0   255
    //lime      128 255 0
    //tangerine 255 128 0
    
    
    
    int insChoice = rand()%5;
    int outsChoice = rand()%5;
    
    CvScalar ins;
    CvScalar outs;
    
    switch (insChoice) {
        case 0:
            ins = CV_RGB(0,255,255);
            break;
        case 1:
            ins = CV_RGB(255,255,0);
            break;
        case 2:
            ins = CV_RGB(0,255,0);
            break;
        case 3:
            ins = CV_RGB(255,0,255);
            break;
        case 4:
            ins = CV_RGB(128,255,0);
            break;
        case 5:
            ins = CV_RGB(255,128,0);
            break;
        default:
            break;
    }
    
    switch (outsChoice) {
        case 0:
            outs = CV_RGB(0,255,255);
            break;
        case 1:
            outs = CV_RGB(255,255,0);
            break;
        case 2:
            outs = CV_RGB(0,255,0);
            break;
        case 3:
            outs = CV_RGB(255,0,255);
            break;
        case 4:
            outs = CV_RGB(128,255,0);
            break;
        case 5:
            outs = CV_RGB(255,128,0);
            break;
        default:
            break;
    }
    
    for( CvSeq* c = contours; c != NULL; c = c->h_next ){
        cvDrawContours(m_sourceImage, c, ins, outs, 1, 25, 8);
    }
    
    cvReleaseMemStorage(&contour_storage);
    
#endif
    
    cvReleaseImage(&sourceGrey);
    cvReleaseImage(&sobelised);
    cvReleaseImage(&threshed);
    cvReleaseImage(&equalised);
    
    /*
     
     CvSeq* first_contour = NULL;
     
     int Nc = cvFindContours(
     img_edge,
     storage,
     &first_contour,
     sizeof(CvContour),
     CV_RETR_LIST );
     
     int n=0;
     printf( "Total Contours Detected: %d\n", Nc );
     CvScalar red = CV_RGB(250,0,0);
     CvScalar blue = CV_RGB(0,0,250);
     
     for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ){
     cvCvtColor( img_8uc1, img_8uc3, CV_GRAY2BGR );
     cvDrawContours(
     img_8uc3,
     c,
     red,		// Red
     blue,		// Blue
     1,			// Vary max_level and compare results
     2,
     8 );
     printf( "Contour #%dn", n );
     cvShowImage( "Contours 2", img_8uc3 );
     printf( " %d elements:\n", c->total );
     for( int i=0; itotal; ++i ){
     CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, c, i );
     printf(" (%d,%d)\n", p->x, p->y );
     }
     cvWaitKey();
     n++;
     }

     
     */
    
    //IplImage* convolved = cvCreateImage(cvGetSize(opImage), IPL_DEPTH_8U, 3);
        
    //m_sourceImage = cvCloneImage(source);
    //m_sourceImage = cvCreateImage(cvGetSize(opImage), IPL_DEPTH_8U, 3);
    //cvConvertScaleAbs(result, m_sourceImage, 1,0);
    //cvReleaseImage(&source);
    //cvReleaseImage(&convolved);
    //cvReleaseImage(&opImage);
    
    processingFinished = true;
    
    return processingFinished;
}



JNIEXPORT
void 
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_setWorkingDir(JNIEnv* env, jobject thiz, jstring javaString){
    
    const char *nativeString = env->GetStringUTFChars(javaString, 0);
    
    pwd = (char*)nativeString;
    
    //env->ReleaseStringUTFChars(javaString, nativeString);
    
    return;
}

JNIEXPORT
void
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_doGrayscaleTransform(JNIEnv* env,
                                                                           jobject thiz){

    IplImage* r = cvCreateImage( cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1 ); 
    IplImage* g = cvCreateImage( cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1 ); 
    IplImage* b = cvCreateImage( cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1 );
    
    
    // Split image onto the color planes. 
    cvSplit( m_sourceImage, b, g, r, NULL );
    
    // Temporary storage
    IplImage* s = cvCreateImage(cvGetSize(m_sourceImage), IPL_DEPTH_8U, 1 );
    
    // Release the source image
    if (m_sourceImage) {
		cvReleaseImage(&m_sourceImage);
		m_sourceImage = 0;
	}
    
    // Add equally weighted rgb values. 
    cvAddWeighted( r, 1./3., g, 1./3., 0.0, s ); 
    cvAddWeighted( s, 2./3., b, 1./3., 0.0, s );
    
    // Merge the 4 channel to an BGRA image
    m_sourceImage = cvCreateImage(cvGetSize(s), 8, 3);
    
    cvMerge(s, s, s, NULL, m_sourceImage);
    
    cvReleaseImage(&r); 
    cvReleaseImage(&g); 
    cvReleaseImage(&b); 
    cvReleaseImage(&s);
    
    return;
}

// Given an integer array of image data, load a float array.
// It is the responsibility of the caller to release the float image.
float* getFloatImageFromIntArray(JNIEnv* env, jintArray array_data, 
                                 jint width, jint height){
    // Load Image
    
    int *pixels = env->GetIntArrayElements(array_data, 0);
    
    float *pixelsImg = new float [width*height];
    
    for (int y = 0; y < height; y ++) {
        
        for (int x = 0; x < width; x++) {
            pixelsImg[x+y*width] = (float)((char)pixels[x+y*width] & 0xFF);
            //pixelsImg[x+y*width*3+1] = (float)(pixels[origX+y*width] >> 8 & 0xFF);
            //pixelsImg[x+y*width*3+2] = (float)(pixels[origX+y*width] >> 16 & 0xFF);
        }
    }
    
    for (int i = 0; i < width; i ++) {
        char buffer[32];
        sprintf(buffer, "@ pixel no. %i -> %f", i, pixelsImg[i]);
        LOGV((char*)buffer,false);
    }
	
    
    
//    LOGV((char*)"checking source int pixels",false);
    
    
    
    
//    float * pixelsAsFloats= new float[width*height];
//
//    for (int i = 0; i < width*height; i++) {
//        pixelsAsFloats[i] = (float)pixels[i];
//    }
    
//    for (int i = 0; i < width; i++) {
//        for (int j = 0; j < height; j++) {
//            pixelsAsFloats[(j*width) + i] = (float)pixels[(j*width) + i];
//        }
//    }
    
    //clean up the jni environment
    env->ReleaseIntArrayElements(array_data, pixels, 0);
    
    return pixelsImg;
}


// Given an integer array of image data, load an IplImage.
// It is the responsibility of the caller to release the IplImage.
IplImage* getIplImageFromIntArray(JNIEnv* env, jintArray array_data, 
								  jint width, jint height) {
	// Load Image
	int *pixels = env->GetIntArrayElements(array_data, 0);
	if (pixels == 0) {
		LOGE("Error getting int array of pixels.");
		return 0;
	}
	
	IplImage *image = loadPixels(pixels, width, height);
	env->ReleaseIntArrayElements(array_data, pixels, 0);
	if(image == 0) {
		LOGE("Error loading pixel array.");
		return 0;
	}
	
	return image;
}


// Generate and return a boolean array from the source image.
// Return 0 if a failure occurs or if the source image is undefined.
JNIEXPORT
jbyteArray
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_getSourceImage(JNIEnv* env,
                                                                     jobject thiz)
{
    
	if (m_sourceImage == 0) {
		LOGE("Error source image was not set.");
		return 0;
	}
	
	CvMat stub;
    CvMat *mat_image = cvGetMat(m_sourceImage, &stub);
    int channels = CV_MAT_CN( mat_image->type );
    int ipl_depth = cvCvToIplDepth(mat_image->type);
    
	WLNonFileByteStream *strm = new WLNonFileByteStream();
    loadImageBytes(mat_image->data.ptr, mat_image->step, mat_image->width,
                   mat_image->height, ipl_depth, channels, strm);
	
	int imageSize = strm->GetSize();
    
    //if you wanted to return an array of 1's and 0's ()
    /*
     jbooleanArray res_array = env->NewBooleanArray(imageSize);
     if (res_array == 0) {
     LOGE("Unable to allocate a new boolean array for the source image.");
     return 0;
     }
     env->SetBooleanArrayRegion(res_array, 0, imageSize, (jboolean*)strm->GetByte());
     */
    
    jbyteArray res_array = env->NewByteArray(imageSize);
    if (res_array == 0) {
        LOGE("Unable to allocate a new byte array for the source image.");
        return 0;
    }
    env->SetByteArrayRegion(res_array, 0, imageSize, (jbyte*)strm->GetByte());
    
	strm->Close();
	//SAFE_DELETE(strm);
	
	return res_array;
    
}

// Set the source image and return true if successful or false otherwise.
JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_setSourceImage(JNIEnv* env,
                                                                     jobject thiz,
                                                                     jintArray photo_data,
                                                                     jint width,
                                                                     jint height)
{	
        
	// Release the image if it hasn't already been released.
	if (m_sourceImage) {
		cvReleaseImage(&m_sourceImage);
		m_sourceImage = 0;
	}
	
	m_sourceImage = getIplImageFromIntArray(env, photo_data, width, height);
	if (m_sourceImage == 0) {
		LOGE("Error source image could not be created.");
		return false;
	}
	
	return true;

#endif
}

JNIEXPORT
jboolean
JNICALL
Java_com_example_videostreamprocessor_VideoActivity_imageProcessingHasFinished(JNIEnv* env,
                                                                                 jobject thiz){
    return processingFinished;
}


JNIEXPORT jstring JNICALL
Java_com_example_videostreamprocessor_VideoActivity_stringFromJNI(JNIEnv* env, jobject thiz){
    //initialise the random seed for neonise functions (used to pick NEON colours)
    srand(time(NULL)); 
    
    #ifdef USINGNEON
    //do a little bit of simple float arithmetric (vector by scalar)
    //if it runs, and computes the correct result... we know it works!
    
    float* src = new float[1];
    src[0] = 1.5f;
    float* dest = new float[1];
    addc_float_c(dest, src, 1.0f, 1);
    
    if (dest[0] == 2.5f) {
        free(src);
        free(dest);
        return env->NewStringUTF("Hello from JNI! (ps... neon can compute floats too!)");
    }
    delete(src);
    delete(dest);
    
    return env->NewStringUTF("Hello from JNI! (but... neon can't compute floats :( )");
    #else
    return env->NewStringUTF("Hello from JNI! (but... neon isn't being used )");
    #endif
    
}


/*
 * End of android specific stuff
 */


