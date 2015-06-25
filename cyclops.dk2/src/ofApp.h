#pragma once
#define _WINSOCKAPI_
#include "ofxBlackmagic.h"
#define WIN32_LEAN_AND_MEAN
#include "ofMain.h"
#include "ofxOculusDK2.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	
		vector<shared_ptr<ofxBlackmagic::Input> > inputs;

		bool remapVideoBuffer;
		ofShader uvShader;
		ofTexture mapTexture;
		ofTexture videoTexture;
		ofFbo equirectangular;

		ofShader highlightShader;

		ofSpherePrimitive sphere;
		ofCamera camera;

		ofxOculusDK2 oculusRift;

		ofRectangle oculusViewport;
		ofRectangle previewViewport;
};
