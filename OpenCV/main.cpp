#define _CRT_SECURE_NO_WARNINGS

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/core.hpp>
#include <chrono>

#include "frangi.h"
#include "houghLineP.h"

#include <iostream>
#include <ctype.h>

using namespace cv;
using namespace std;
using namespace std::chrono;

Mat image;

bool backProj = true;
bool selectObject = false;
int tracking = 0;
Point origin;
Rect selection;



string type2str(int type) {
	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}


//Will be in a class and not global at a later date
Mat lines, circles;
int width;

int bresLineTracking(Point2f l1, Point2f l2) {
	int _x = l1.x;
	int _y = l1.y;

	int dx = l1.x - l2.x;
	int dy = l1.y - l2.y;

	int dLong = abs(dx);
	int dShort = abs(dy);

	int offLong = dx < 0 ? 1 : -1;
	int offShort = dy < 0 ? width : -width;

	if (dLong < dShort) {
		swap(dShort, dLong);
		swap(offShort, offLong);
	}

	int error = dLong / 2;
	int index = _y * width + _x;
	int offSet[] = { offLong, offLong + offShort };
	int absd[] = { dShort, dShort - dLong };

	//Tracks along the line and corrects errors such as going over an edge
	for (int k = 0; k <= dLong; k++) {
		circles.ptr<unsigned char>()[index] = 255; //Draw after calculating length and curve of line
		const int tooBig = error >= dLong;
		index += offSet[tooBig];
		error += absd[tooBig];
	}
	return index;
}

int main() {
	//Declarations
	VideoCapture cap;
	Rect trackWindow;
	const int MAX_ORIENTATION = 3;

	int slider = 20, q = 1, w = 2, e = 15, t = 130;
	namedWindow("slider", 0);
	createTrackbar("trackbar", "slider", &q, 20);
	createTrackbar("trackbar2", "slider", &w, 20);
	createTrackbar("trackbar3", "slider", &e, 50);
	createTrackbar("Threshold", "slider", &t, 500);
	createTrackbar("density", "slider", &slider, 200);

	Mat frame, hsv, yuv, gray, thresh, mask, gray32, gabor, gaborWeights, kernel, gaborFinal;

	Mat bHist, gHist, rHist;
	int histH = 400, histW = 512;

	char* location = new char[100];
	char* output = new char[100];

	//while (true) {
	for (int im = 0; im < 105; im++) {

		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		sprintf(location, "../dataset/%i.png", im);
		sprintf(output, "../complete/%i-spots.png", im);

		//Load and gray scale the image
		frame = imread(location);

		GaussianBlur(frame, frame, Size(7, 7), 0);

		//GaussianBlur(frame, frame, Size(5, 5), 0, 0);
		hsv = Mat(frame.rows, frame.cols, CV_8UC3);
		yuv = Mat(frame.rows, frame.cols, CV_8UC3);
		circles = Mat(frame.rows, frame.cols, CV_8UC1);
		thresh = Mat(frame.rows, frame.cols, CV_8UC1);
		mask = Mat(frame.rows, frame.cols, CV_8UC1);
		gray = Mat(frame.rows, frame.cols, CV_8UC1);

		hsv.setTo(Scalar(0, 0, 0));
		thresh.setTo(Scalar(0, 0, 0));
		mask.setTo(Scalar(0, 0, 0));
		circles.setTo(0);

		cvtColor(frame, gray, COLOR_BGR2GRAY);
		cvtColor(frame, yuv, COLOR_BGR2YCrCb);
		cvtColor(frame, hsv, COLOR_BGR2HSV);


		//Algorithm should
		// 1. Locate an outer circle/shape
		// 2. Locate the inner circle if there is any
		// 3. Tell the gradient change from the circle edge to center
		// 4. Tell the overall colour in HSV

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		vector<Point3f> spotLocations;
		vector<Point2f> spotArea;


		double arc, area, circular;
		Scalar colour;
		//int hSpotLess = 7, hSpotMore = 175, sSpot = 120, vSpotMore = 62, vSpotLess = 162, hFreckle = 25, sFreckle = 120, vFreckle = 120;

		int hSpotLess = 7, hSpotMore = 175, sSpotMore = 130, sSpotLess = 255, vSpotMore = 62, vSpotLess = 162;
		int hFreckleLess = 8, hFreckleMore = 5, sFreckleLess = 204, sFreckleMore = 80, vFreckleLess = 105, vFreckleMore = 0;
		//Cycles through different thresholds of the gray scaled image and locates blobs
		for (int i = 1; i < 25; i++) { //Cycles from 0 to 250

			//Saves gray scale
			for (int index = 0; index < frame.cols * frame.rows; index++) {
				if (gray.ptr<unsigned char>()[index] > i * 10) thresh.ptr<unsigned char>()[index] = 250;
			}

			//Locates contours, this includes blobs that could be freckles/spots
			findContours(thresh, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

			vector<Moments> mu(contours.size());	//Area of shape
			vector<Point2f> mc(contours.size());	//Center points

			//Locate all areas with multiple circles of gradient
			for (int j = 0; j < contours.size(); j++) {
				//Get center point of the contour and using "moments"
				mu[j] = moments(contours[j], true);
				mc[j] = Point2f(mu[j].m10 / mu[j].m00, mu[j].m01 / mu[j].m00);

				//Check if the contour is a circular shape
				arc = arcLength(contours[j], true);
				area = contourArea(contours[j], true);
				circular = 2 * CV_2PI*(area / (arc*arc));

				drawContours(frame, contours, j, Scalar(160, 130, 0), 1, LINE_4, hierarchy, 0, Point());

				//Adds anything in the shape of a pimple/freckle to a vector
				if (area > 20 && area < 500 && circular > 0.6) {
					spotLocations.push_back(Point3f(mc[j].x, mc[j].y, 1));
				}

				//Cycles through possible spots and removes multiple circles with the same center point
				//Counts how many circles are surrounding a single location and adds it up
				//More circles means its likely to be a pimple or freckle
				for (int k = 0; k < spotLocations.size(); k++) {
					if( k != spotLocations.size() - 1
						&& mc[j].x > spotLocations[k].x - 2 && mc[j].x < spotLocations[k].x + 2
						&& mc[j].y > spotLocations[k].y - 2 && mc[j].y < spotLocations[k].y + 2) {

						spotLocations[k].z += 1;
						spotLocations.pop_back();
					}
					
				}
			}
			thresh.setTo(0);
		}

		//Aims to seperate pimples and freckles by checking the colour of the area
		for (int i = 0; i < spotLocations.size(); i++) {
			Point2f location(spotLocations[i].x, spotLocations[i].y);

			if (spotLocations[i].z >= 2 
				//&& (hsv.at<Vec3b>(location)[0] <= hSpotLess || hsv.at<Vec3b>(location)[0] >= hSpotMore)
				//&& (hsv.at<Vec3b>(location)[1] <= sSpotLess && hsv.at<Vec3b>(location)[1] >= sSpotMore)
				//&& (hsv.at<Vec3b>(location)[2] <= vSpotLess && hsv.at<Vec3b>(location)[2] >= vSpotMore)
				) {
				circle(frame, location, 4, Scalar(spotLocations[i].z * 100), 2, 8, 0);
			}


		}


		/*
				
					if ((hsv.at<Vec3b>(mc[j])[0] <= hSpotLess || hsv.at<Vec3b>(mc[j])[0] >= hSpotMore)
						&& (hsv.at<Vec3b>(mc[j])[1] <= sSpotLess && hsv.at<Vec3b>(mc[j])[1] >= sSpotMore)
						&& (hsv.at<Vec3b>(mc[j])[2] <= vSpotLess && hsv.at<Vec3b>(mc[j])[2] >= vSpotMore)
						) {

						colour = Scalar(0, 200, 0);
						circle(frame, mc[j], 1, colour, -1, 8, 0);

						drawContours(frame, contours, j, colour, 1, LINE_4, hierarchy, 0, Point());
						//Add first position, z value is the depth of the spot, amount of circles around it
						spotLocations.push_back(Point3f(mc[j].x, mc[j].y, 1));

						//Cycle through spots found so far and remove ones that have already been located
						for (int k = 0; k < spotLocations.size(); k++) {

							if (k != spotLocations.size() - 1
								&& mc[j].x > spotLocations[k].x - 5 && mc[j].x < spotLocations[k].x + 5
								&& mc[j].y > spotLocations[k].y - 5 && mc[j].y < spotLocations[k].y + 5) {

								spotLocations[k].z += 1;
								spotLocations.pop_back();

							}
						}
					
				}
				if (area > 80 && area < 500 && circular > 0.6
					&& (hsv.at<Vec3b>(mc[j])[0] >= hFreckleMore && hsv.at<Vec3b>(mc[j])[0] <= hFreckleLess)
					&& (hsv.at<Vec3b>(mc[j])[1] >= sFreckleMore && hsv.at<Vec3b>(mc[j])[1] <= sFreckleLess)
					&& (hsv.at<Vec3b>(mc[j])[2] >= vFreckleMore && hsv.at<Vec3b>(mc[j])[2] <= vFreckleLess)
					) {

					drawContours(frame, contours, j, Scalar(0, 0, 225), 1, LINE_4, hierarchy, 0, Point());
				
				
				}

			}

			*/
		imwrite(output, frame);

#ifdef Circle
//Amount of edges around the defined circles
		int edges = 8;
		
		float circleStep = CV_2PI / edges;
		int radius = 5;

		spotLocations.size();

		Point2f point1;

		int startPositionX = 300;
		int startPositionY = 150;

		vector<double> curve;
		//width of image so that it cycles back if it goes over the edge
		width = circles.cols;

		double previous = 0, difference = 0, highest = 0, value = 0;
		vector<Point2f> edgePoints;
		Point2f highestLocation;

		for (int k = 0; k < spotLocations.size(); k++) {
			//Places circles that are used to control trajectroy
			
			//circle(frame, spotLocations[k], 1, Scalar(10, 250, 0), -1, 8, 0);
			//cout << spotLocations[i] << endl;

			for (float theta = 0; theta < CV_2PI; theta += circleStep) {

				//Starting point, edge of the line
				point1.x = startPositionX + radius * cos(theta);
				point1.y = startPositionY - radius * sin(theta);

				//Set to the very last point
				previous = hsv.at<Vec3b>(point1.y, point1.x)[1];

				//Cycle through all but the first, which is defined in previous
				for (int j = 1; j < radius; j += 2) {

					for (int i = 0; i < 2; i++) {
						point1.x = startPositionX + (radius - (j + i)) * cos(theta);
						point1.y = startPositionY - (radius - (j + i)) * sin(theta);

						if (hsv.at<Vec3b>(point1.y, point1.x)[1] > 60) 
							value = hsv.at<Vec3b>(point1.y, point1.x)[1] - previous;
						else 
							value = 0;

						difference += value;

						//frame.at<Vec3b>(point1.y, point1.x)[1] = 250;

						previous = hsv.at<Vec3b>(point1.y, point1.x)[1];
					}

					curve.push_back(difference); //Saves line for later analysis

					//Save highest difference in threshold for drawing an edge around the blob for shape analysis
					if (highest < difference) {

						highest = difference;
						highestLocation = point1;
					}

					difference = 0;
					//Calculate the change in gradients
				}

				//Sets the highest location, this saved value is one edge of the shape
				edgePoints.push_back(highestLocation);

				//Reset values
				highest = 0;
				difference = 0;

				//Save average of circle colour

				//Compare difference if not first iteration
				//Should be a smooth change from skin colour to more red

				//Find starting point of curve
				for (int i = 0; i < curve.size(); i++) {
					cout << curve[i] << "\t";
				}

				cout << endl;

				curve.clear();
			}

			//Draws area of spot
			for (int i = 1; i < edgePoints.size(); i++) {
				//bresLineTracking(edgePoints[i-1], edgePoints[i]);
			}

			edgePoints.clear();

		}
		

#endif // Circle stuff

		
		high_resolution_clock::time_point t2 = high_resolution_clock::now();

		//3. Displays the window
		//imshow("Original colour", frame);
		//imshow("Original yuv", hsv);
		//imshow("Thresh", thresh);
		//imshow("mask", gray);
		//imshow("slider", 0);

		auto duration = duration_cast<microseconds>(t2 - t1).count() / 1000;

		cout << "Time: " << duration << endl;
		
		//Hit esc to quit and b to show the background projection
		char c = (char)waitKey(10);
		if (c == 27)
			break;
		}
	return 0;
}