#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "FreeImagePlus.h"
#include "iostream"

#define GFRAMES 5;
#define GWIDTH 400

using namespace std;
using namespace cv;

vector<Rect> doDetection(Mat& input);
vector<Mat> sprites;
void showImage(Mat& mat);
Mat renderFrame(Mat& img,vector<Rect>& eyes,Mat& eye);
Mat overlayImage(Mat& background, Mat& foreground, Point location);
void renderFromImage(QString& filename);
void renderFromVideo(QString& filename);
QFileInfo fileInfo;

int main(int argc, char *argv[])
{

   // QCoreApplication a(argc, argv);
    Q_INIT_RESOURCE(resources);

    QString filename = "/home/fatty/gw7K6H7.png";

    if (argc>1) {
        filename = QString(argv[1]);
    }

    fileInfo = QFileInfo(filename);
    if (!fileInfo.isFile()) {
        cerr << filename.toStdString() << " does not exist. " << endl;
        exit(1);
    }
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filename);

    cerr << type.name().toStdString() << endl;

    QFile googly(QString(":/img/img/googlysprite.png"));
    googly.open(QFile::ReadOnly);
    QByteArray qb = googly.readAll();

    Mat googlySprite = imdecode(vector<char>(qb.begin(),qb.end()), IMREAD_UNCHANGED);
    for (int i=0;i<5;i++) {
        Mat sprite;
        Rect snatch(i*399,0,399,399);
        try {
            sprite = googlySprite(snatch);

        } catch(int ex) {
            cerr << "FUCKEDY UP" << endl;
            cerr << ex << endl;
            exit(1);
        }
        sprites.push_back(sprite);
    }

    if (type.name().startsWith("image")) {
        renderFromImage(filename);
    } else if (type.name().startsWith("video")) {
        renderFromVideo(filename);
    }

    cout << "Finished!\n";

    //return a.exec();
}

void renderFromImage(QString& filename) {
    VideoWriter vw;
    Mat image = imread(filename.toStdString(),IMREAD_ANYCOLOR);
    vw.open("/home/fatty/test.mp4",CV_FOURCC('X','2','6','4'),20.0,Size(image.cols,image.rows),true);


    vector<Rect> eyes = doDetection(image);
    if (eyes.empty()) {
        cerr << "Ain;t got no eyes, lieutenant dan" << endl;
        exit(1);
    }
    cout << "rendering... ";
    for (int i=0;i<20;i++) {
        Mat frame = renderFrame(image,eyes,sprites[i % 5]);
        vw << frame;
        //cout << "." << flush;

    }
    vw.release();
}

void renderFromVideo(QString& filename) {

    VideoCapture vc(filename.toStdString());
    int codec = static_cast<int>(vc.get(CV_CAP_PROP_FOURCC));
    int frames = static_cast<int>(vc.get(CV_CAP_PROP_FRAME_COUNT));

    if (!codec) {
        cerr << filename.toStdString() << "is not supported. Welp!" << endl;
        exit(1);
    }

    Size S = Size((int) vc.get(CV_CAP_PROP_FRAME_WIDTH), (int) vc.get(CV_CAP_PROP_FRAME_HEIGHT));
    VideoWriter vw("/home/fatty/output.mp4",codec,vc.get(CV_CAP_PROP_FPS), S, true);
    Mat inFrame;
    int frameCount=0;
    vector<Rect> eyes;
    cerr << "Beggining processing..." << endl;
    namedWindow("VIDJAS YALL");
    for(;;) {
        vc >> inFrame;
        if (inFrame.empty()) {
            break;
        }
        eyes = doDetection(inFrame);
        if (!eyes.empty()) {
            Mat f = renderFrame(inFrame,eyes,sprites[frameCount % 5]);
            vw << f;
        } else {
            vw << inFrame;
        }

        frameCount++;
        if (frameCount % 20==0) {
            cerr << "Rendering frame " << frameCount << " of " << frames << "(" << (double)((frameCount/frames)*100) << "%)" << endl;
        }
    }
    vw.release();
}

Mat renderFrame(Mat& img,vector<Rect>& eyes,Mat& eye) {
    Mat background;
    Mat final;
    if (img.channels()!=eye.channels()) {
        cvtColor(img,background,CV_BGR2BGRA);
    } else {
        background = img.clone();
    }
    foreach (Rect detected,eyes) {
        Size rst = Size(detected.width,detected.height);
        Mat tmp;
        resize(eye,tmp,rst,0,0,INTER_LINEAR);
        background = overlayImage(background,tmp,Point(detected.x,detected.y));
        //Mat roi = background(Rect(detected.x,detected.y,detected.width,detected.height));
        //addWeighted(roi, 0, tmp, 1, 0, roi);
        //tmp.copyTo(background(Rect(detected.x,detected.y,detected.width,detected.height)));
    }

    cvtColor(background,final,CV_RGBA2RGB);
    return final;
}

void showImage(Mat& mat) {
    namedWindow("wat",WINDOW_AUTOSIZE);
    imshow("wat",mat);
    waitKey(0);
}

vector<Rect> doDetection(Mat& input) {

    Mat grayScale;
    cvtColor( input, grayScale, CV_BGR2GRAY );

    vector<Rect> faces;
    vector<Rect> allEyes;

    CascadeClassifier face("/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml");
    CascadeClassifier eyes("/usr/share/opencv/haarcascades/haarcascade_eye.xml");

    face.detectMultiScale(grayScale,faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );
    foreach (Rect foundFace,faces) {

        vector<Rect> eyeBallses;
        eyes.detectMultiScale(grayScale(foundFace),eyeBallses,2, 2.0, 1.0 );

        foreach (Rect eye,eyeBallses) {
            eye.x+=foundFace.x;
            eye.y+=foundFace.y;
            allEyes.push_back(eye);
        }
    }

    return allEyes;

}

Mat overlayImage(Mat& background, Mat& foreground, Point location) {
  Mat output = background.clone();


  // start at the row indicated by location, or at row 0 if location.y is negative.
  for(int y = std::max(location.y , 0); y < background.rows; ++y)
  {
    int fY = y - location.y; // because of the translation

    // we are done of we have processed all rows of the foreground image.
    if(fY >= foreground.rows)
      break;

    // start at the column indicated by location,

    // or at column 0 if location.x is negative.
    for(int x = std::max(location.x, 0); x < background.cols; ++x)
    {
      int fX = x - location.x; // because of the translation.

      // we are done with this row if the column is outside of the foreground image.
      if(fX >= foreground.cols)
        break;

      // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
      double opacity =
        ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3])

        / 255.;


      // and now combine the background and foreground pixel, using the opacity,

      // but only if opacity > 0.
      for(int c = 0; opacity > 0 && c < output.channels(); ++c)
      {
        unsigned char foregroundPx =
          foreground.data[fY * foreground.step + fX * foreground.channels() + c];
        unsigned char backgroundPx =
          background.data[y * background.step + x * background.channels() + c];
        output.data[y*output.step + output.channels()*x + c] =
          backgroundPx * (1.-opacity) + foregroundPx * opacity;
      }
    }
  }
  return output;
}

