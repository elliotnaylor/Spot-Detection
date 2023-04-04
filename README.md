# Spot-Detection
This was originally developed for a company looking to detect unhealthy skin using a number of computer vision algorithms. This was one of the decided upon techniques, which can be used to detect pimples and wrinkles. No machine learning techniques are used for this technique because of a lack of available data at the time of creation.

## Discussion
This is an algorithm designed for the detection of acne using techniques including thresholding, contours, and moments. The algorithm is tested using the AGES dataset and gets an accuracy of 76.17%.

Firstly the image is gaussian blurred using an 11 by 11 kernel to remove smaller areas that are not of interest to the algorihtm, this improves processing speed and accuracy. The image is then converted to a gray scale

```
//Cycles through different thresholds of the gray scaled image and locates blobs
for (int i = 1; i < 25; i++) { //Cycles from 0 to 250
  //Saves gray scale
  for (int index = 0; index < frame.cols * frame.rows; index++) {
    if (gray.ptr<unsigned char>()[index] > i * 10) thresh.ptr<unsigned char>()[index] = 250;
  }
}
```
```
//Locates contours, this includes blobs that could be freckles/spots
findContours(thresh, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

vector<Moments> mu(contours.size());	//Area of shape
vector<Point2f> mc(contours.size());	//Center points

for (int j = 0; j < contours.size(); j++) {

  //Get center point of the contour and using "moments"
  mu[j] = moments(contours[j], true);
  mc[j] = Point2f(mu[j].m10 / mu[j].m00, mu[j].m01 / mu[j].m00);
```
|![image](https://user-images.githubusercontent.com/22525909/229362388-34847524-a697-4dae-b914-29342453a661.png)|![image](https://user-images.githubusercontent.com/22525909/229362400-0945319f-e8e9-4722-ad85-7c02d4095b07.png)|![image](https://user-images.githubusercontent.com/22525909/229362482-d235364b-c9c1-4319-9209-45306c1b9d04.png)|![image](https://user-images.githubusercontent.com/22525909/229362413-25060d4f-3fe1-45ed-bf64-00aa6be7c4ba.png)|

## Conclusion
