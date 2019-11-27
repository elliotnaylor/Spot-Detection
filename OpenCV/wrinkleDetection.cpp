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

class WrinkleDetection : ParallelLoopBody {
	WrinkleDetection();

	virtual void operator ()(const Range& range) const {
		
	}

};