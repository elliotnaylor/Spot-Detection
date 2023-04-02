# Spot-Detection
# Introduction
Spots or acne are a skin condition that effects people of all ages and occurs when hair follicles become plugged with dead skin cells and oil. These are likely to appear on the face, chest upper back and shoulders, but can appear anywhere on the body. Acne is separated into multiple different types of spots in the form of whiteheads, blackheads and pimples. Out of these there is likely to be only enough data to detect pimples because their colour and shape are distinct compared to the others that would require specialised equipment or massive amounts of images to detect at a high accuracy. Acne is specifically triggered by stress, diet and hormones and as such the detection of this would allow for easier analysis of a person without seeing a doctor.

# Method
This is an algorithm designed for the detection of acne using thresholding, contours, and moments. 

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
![image](https://user-images.githubusercontent.com/22525909/229362388-34847524-a697-4dae-b914-29342453a661.png)

![image](https://user-images.githubusercontent.com/22525909/229362400-0945319f-e8e9-4722-ad85-7c02d4095b07.png)

![image](https://user-images.githubusercontent.com/22525909/229362404-bc7bfc25-b73b-4460-8268-97ea8a74725c.png)

![image](https://user-images.githubusercontent.com/22525909/229362413-25060d4f-3fe1-45ed-bf64-00aa6be7c4ba.png)


