// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"

#include "thread"
#include "chrono"

using namespace std;

void centering_transform(Mat img) {
	//expects floating point image
	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			img.at<float>(i, j) = ((i + j) & 1) ? -img.at<float>(i, j) : img.at<float>(i, j);
		}
	}
}

Mat frequency_domain_filter(Mat src, int nr, int val) {
	//convert input image to float image
	Mat srcf;
	src.convertTo(srcf, CV_32FC1);
	//centering transformation
	centering_transform(srcf);
	//perform forward transform with complex image output
	Mat fourier;
	dft(srcf, fourier, DFT_COMPLEX_OUTPUT);
	//split into real and imaginary channels
	Mat channels[] = { Mat::zeros(src.size(), CV_32F), Mat::zeros(src.size(), CV_32F) };
	split(fourier, channels); // channels[0] = Re(DFT(I)), channels[1] = Im(DFT(I))
	//calculate magnitude and phase in floating point images mag and phi
	Mat mag, phi;
	magnitude(channels[0], channels[1], mag);
	phase(channels[0], channels[1], phi);
	float max = 0;

	for (int i = 0; i < mag.rows; i++) {
		for (int j = 0; j < mag.cols; j++) {
			if (mag.at<float>(i, j) > max) {
				max = mag.at<float>(i, j);
			}
		}
	}

	//display the phase and magnitude images here
	//imshow("Phase Image", phi);
	//Mat mag2(mag.rows, mag.cols, CV_32FC1);
	//for (int i = 0; i < mag.rows; i++) {
	//	for (int j = 0; j < mag.cols; j++) {
	//		mag2.at<float>(i, j) = log2(mag.at<float>(i, j)) + 1;
	//	}
	//}
	//Mat mag_normalized;
	//normalize(mag2, mag_normalized, 0, 255, NORM_MINMAX, CV_8UC1);
	//imshow("Magnitude Image", mag_normalized);

	//insert filtering operations on Fourier coefficients here
	//store in real part in channels[0] and imaginary part in channels[1]

	// ideal low pass
	if (nr == 0) {
		int R = val;//10;
		for (int i = 0; i < fourier.rows; i++)
		{
			for (int j = 0; j < fourier.cols; j++)
			{
				int dist = (i - fourier.rows / 2) * (i - fourier.rows / 2) + (j - fourier.cols / 2) * (j - fourier.cols / 2);
				if (dist > R * R)
				{
					//channels[0].at<float>(i, j) = 0;
					//channels[1].at<float>(i, j) = 0;
					mag.at<float>(i, j) = 0;
				}
			}
		}
	}
	// ideal high pass
	else if (nr == 1) {
		int R = val;//70;
		for (int i = 0; i < fourier.rows; i++)
		{
			for (int j = 0; j < fourier.cols; j++)
			{
				int dist = (i - fourier.rows / 2) * (i - fourier.rows / 2) + (j - fourier.cols / 2) * (j - fourier.cols / 2);
				if (dist < R * R)
				{
					//channels[0].at<float>(i, j) = 0;
					//channels[1].at<float>(i, j) = 0;
					mag.at<float>(i, j) = 0;
				}
			}
		}
	}
	// gaussian low pass
	else if (nr == 2) {
		float A = val;//20;
		for (int i = 0; i < fourier.rows; i++)
		{
			for (int j = 0; j < fourier.cols; j++)
			{
				float expon = exp(-((i - fourier.rows / 2) * (i - fourier.rows / 2) + (j - fourier.cols / 2) * (j - fourier.cols / 2)) / (A * A));

				//channels[0].at<float>(i, j) *= expon;
				//channels[1].at<float>(i, j) *= expon;
				mag.at<float>(i, j) *= expon;

			}
		}
	}
	// gaussian high pass
	else if (nr == 3) {
		float A = val;//20;
		for (int i = 0; i < fourier.rows; i++)
		{
			for (int j = 0; j < fourier.cols; j++)
			{
				float expon = exp(-((i - fourier.rows / 2) * (i - fourier.rows / 2) + (j - fourier.cols / 2) * (j - fourier.cols / 2)) / (A * A));

				channels[0].at<float>(i, j) *= (1 - expon);
				channels[1].at<float>(i, j) *= (1 - expon);
				mag.at<float>(i, j) *= (1 - expon);

			}
		}
	}

	//for (int i = 0; i < mag.rows; i++) {
	//	for (int j = 0; j < mag.cols; j++) {
	//		channels[0].at<float>(i, j) = mag.at<float>(i,j) * cos(phi.at<float>(i,j));
	//		channels[1].at<float>(i, j) = mag.at<float>(i, j) * sin(phi.at<float>(i, j));
	//	}
	//}
	Mat mag2(mag.rows, mag.cols, CV_32FC1);
	for (int i = 0; i < mag.rows; i++) {
		for (int j = 0; j < mag.cols; j++) {
			mag2.at<float>(i, j) = log2(mag.at<float>(i, j) + 1);
		}
	}
	Mat_<uchar> mag_normalized(mag.rows, mag.cols);
	float maxLog = log2(max + 1);

	for (int i = 0; i < mag.rows; i++) {
		for (int j = 0; j < mag.cols; j++) {
			mag_normalized(i, j) = mag2.at<float>(i, j) * 255 / maxLog;
		}
	}

	//normalize(mag2, mag_normalized, 0, 255, NORM_MINMAX, CV_8UC1);
	//imshow("Magnitude", mag_normalized);
	//waitKey(0);

	//perform inverse transform and put results in dstf
	Mat dst, dstf;
	merge(channels, 2, fourier);
	dft(fourier, dstf, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);
	//inverse centering transformation
	centering_transform(dstf);
	//normalize the result and put in the destination image
	normalize(dstf, dst, 0, 255, NORM_MINMAX, CV_8UC1);
	//Note: normalizing distorts the resut while enhancing the image display in the range [0,255].
	//For exact results (see Practical work 3) the normalization should be replaced with convertion:
	//dstf.convertTo(dst, CV_8UC1);
	Mat final(dst.rows, mag_normalized.cols + dst.cols, CV_8UC1);
	for (int i = 0; i < final.rows; i++) {
		for (int j = 0; j < mag_normalized.cols; j++) {
			final.at<uchar>(i, j) = mag_normalized.at<uchar>(i, j);
		}
	}
	for (int i = 0; i < final.rows; i++) {
		for (int j = 0; j < dst.cols; j++) {
			final.at<uchar>(i, mag_normalized.cols + j) = dst.at<uchar>(i, j);
		}
	}
	return final;
}

int main()
{
	int option;
	cin >> option;
	Mat_<uchar> img = imread("Images/cameraman.bmp", IMREAD_GRAYSCALE);

	switch (option)
	{
	case 1:
		for (int v = 10; v <= 80; v += 2) {
			//imshow("Low Pass Filter Result", frequency_domain_filter(img, 0, v));
			//waitKey(0);
			imwrite(std::string("Images/low_pass/low_pass_"+ std::to_string(v)+".bmp"), frequency_domain_filter(img, 0, v));
		}
		break;
	case 2:
		for (int v = 10; v <= 80; v += 2) {
			//imshow("High Pass Filter Result", frequency_domain_filter(img, 1, v));
			//waitKey(0);
			imwrite("Images/high_pass/high_pass_" + std::to_string(v) + ".bmp", frequency_domain_filter(img, 1, v));
		}
		break;
	case 3:
		for (int v = 10; v <= 80; v += 2) {
			//imshow("Gaussian Low Pass Filter Result", frequency_domain_filter(img, 2, v));
			//waitKey(0);
			imwrite("Images/gaussian_low_pass/gaussian_low_pass_" + std::to_string(v) + ".bmp", frequency_domain_filter(img, 2, v));
		}
		break;
	case 4:
		for (int v = 10; v <= 80; v += 2) {
			//imshow("Gaussian High Pass Filter Result", frequency_domain_filter(img, 3, v));
			//waitKey(0);
			imwrite("Images/gaussian_high_pass/gaussian_high_pass_v222" + std::to_string(v) + ".bmp", frequency_domain_filter(img, 3, v));
		}
		break;
	default:
		break;
	}
	return 0;
}