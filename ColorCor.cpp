#include "curl/curl.h"
#include <iostream>
#include <armadillo>
#include <vector>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

const int num_threads = 4;
cv::Mat img;

using namespace cv;
using namespace std;
using namespace arma;

int* rgb_fix(int r, int g, int b);
void colorCor(cv::Mat img, int start, int end);
int rgbValChecker(int* RGB_d_arr, int pos);

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    vector<uchar> *stream = (vector<uchar>*)userdata;
    size_t count = size * nmemb;
    stream->insert(stream->end(), ptr, ptr + count);
    return count;
}

//function to retrieve the image as cv::Mat data type
cv::Mat curlImg(const char *img_url, int timeout=10)
{
    vector<uchar> stream;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, img_url); //the img url
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // pass the writefunction
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream); // pass the stream ptr to the writefunction
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout); // timeout if curl_easy hangs,
    CURLcode res = curl_easy_perform(curl); // start curl
    curl_easy_cleanup(curl); // cleanup
    return imdecode(stream, -1); // 'keep-as-is'
}


int main(int argc, char** argv) {

  if(argc > 2) {
    if(strcmp( argv[1], "-l") == 0) {     // local file
      img = imread(argv[2], CV_LOAD_IMAGE_COLOR);
    } else if (strcmp( argv[1], "-u") == 0) {   // from url
      img = curlImg(argv[2]);
    }
  } else {
    cout << "No parameters." << endl;
  }

  std::thread t[num_threads];

  if(! img.data )                              // Check for invalid input
  {
      cout <<  "Could not open or find the image" << std::endl ;
      return -1;
  }

  for(int i = 0; i < num_threads; i++) {
    int start = (img.cols/num_threads)*i;
    int end = ((img.cols/num_threads))*(i+1);
    if(i == num_threads-1) {
      end = img.cols;
    }
    t[i] = std::thread(colorCor, img, start, end);
  }

   for(int i = 0; i < num_threads; i++) {
     t[i].join();
   }

  namedWindow( "Display window", WINDOW_NORMAL );// Create a window for display.
  imshow( "Display window", img );                   // Show our image inside it.

  waitKey(0);

  return 0;
}

void colorCor(cv::Mat img, int start, int end) {
  for(int y=start;y<end;y++)
  {
      for(int x=0;x<img.rows;x++)
      {
        // get pixel
        Vec3b color = img.at<Vec3b>(x,y);

        float b = color.val[0];
        float g = color.val[1];
        float r = color.val[2];

        // // within the for loop
        int* rgb = new int[3];
        rgb = rgb_fix(r,g,b);

        color.val[0] = rgb[2]; // b
        color.val[1] = rgb[1]; // g
        color.val[2] = rgb[0]; // r

        // set pixel
        img.at<Vec3b>(x,y) = color;
        delete[] rgb;
      }
  }
}

int* rgb_fix(int r, int g, int b) {

  // create 3x1 matrix for the rgb values that will be altered
  mat RGB(3,1);
  RGB << r << endr
      << g << endr
      << b << endr;

  // 3x3 matrix that must be multipled with rgb to get the LMS value
  mat lms_cor(3,3);
  lms_cor << 17.8824 << 43.5161 << 4.11935 << endr
          << 3.45565 << 27.1554 << 3.86714 << endr
          << 0.02996 << 0.184309 << 1.46709 << endr;

  // LMS = corrected matrix times rgb of the pixel
  mat LMS(3,1);
  LMS = lms_cor * RGB;

  // 3x3 matrix that must be multipled with LMS to get the LMS_cb value
  mat LMS_cb_corr(3,3);
  LMS_cb_corr << 1 << 0 << 0 << endr
              << 0.494207 << 0 << 1.24827 << endr
              << 0 << 0 << 1 << endr;

  // LMS_cb = corrected matrix times LMS
  mat LMS_cb(3,1);
  LMS_cb = LMS_cb_corr * LMS;

  // 3x3 matrix that must be multipled with LMS_cb to get the RGB_cb value
  mat RGB_cb_corr(3,3);
  RGB_cb_corr << 0.0809445 << -0.130505 << 0.1167211 << endr
         << -0.0102485 << 0.0540193 << -0.113615 << endr
         << -0.0000365 << -0.0041216 << 0.6935114 << endr;

   // LMS_cb = corrected matrix times LMS
   mat RGB_cb(3,1);
   RGB_cb = RGB_cb_corr * LMS_cb;

   // RGB_e = RGB - rgb_cb
   mat RGB_e(3,1);
   RGB_e = RGB - RGB_cb;

   mat RGB_s_corr(3,3);
   RGB_s_corr << 0 << 0 << 0 << endr
              << 0.7 << 1 << 0 << endr
              << 0.7 << 0 << 1 << endr;
   mat RGB_s(3,1);
   RGB_s = RGB_s_corr * RGB_e;

   mat RGB_d(3,1);
   RGB_d = RGB + RGB_s;

   int* RGB_d_arr = new int[3];
   for(int i = 0; i < 3; i++) {
     RGB_d_arr[i] = RGB_d.memptr()[i];
   }

   RGB_d_arr[0] = rgbValChecker(RGB_d_arr, 0);
   RGB_d_arr[1] = rgbValChecker(RGB_d_arr, 1);
   RGB_d_arr[2] = rgbValChecker(RGB_d_arr, 2);

   return RGB_d_arr;
}

int rgbValChecker(int* RGB_d_arr, int pos) {
  if(RGB_d_arr[pos] < 0) {
    return 0;
  } else if (RGB_d_arr[pos] > 255) {
    return 255;
  } else {
    return RGB_d_arr[pos];
  }
}
