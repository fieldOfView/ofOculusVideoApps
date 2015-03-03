#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	camWidth 		= 720;	// try to grab at this size. 
	camHeight 		= 625;

	int renderWidth = 2048;
	int renderHeight = 1024;

	oculusViewport = ofRectangle(0, 0, 800,800);
	previewViewport = ofRectangle(800, 0, 1280-800, 800);

	ofSetVerticalSync(true);
	ofBackground(0);
	ofSetLogLevel(OF_LOG_NOTICE);

    //we can now get back a list of devices. 
	ofVideoGrabber grabber;
	vector<ofVideoDevice> devices = grabber.listDevices();

	ofShortPixels pixels;
	ofTexture mapTexture;

	vidGrabbers.clear();
	videoImages.clear();
	mapTextures.clear();
    for(int i = 0; i < devices.size(); i++){
        if( devices[i].bAvailable ){
			ofLogNotice(devices[i].id + ": " + devices[i].deviceName);
			// load mapfile
			if( !ofLoadImage(pixels, "maps/map_"+ofToString(i)+".png")) {
				ofLogWarning("Map failed to load, ignoring device");
				continue;
			}

			//load the pixels to a texture
			mapTexture.allocate(pixels.getWidth(), pixels.getHeight(), GL_RGBA16, true);
			mapTexture.loadData(pixels);
			mapTextures.push_back(mapTexture);


			// set up grabber
			ofVideoGrabber vidGrabber;
			vidGrabber.setDeviceID(i);
			vidGrabber.setDesiredFrameRate(30);
			vidGrabber.initGrabber(camWidth,camHeight);
			vidGrabbers.push_back(vidGrabber);

			// prepare grabber texture
			ofImage videoImage;
			videoImage.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
			videoImages.push_back(videoImage);
        }else{
			ofLogWarning(devices[i].id + ": " + devices[i].deviceName + " - unavailable");
        }
	}

	// load shaders and exit if not succseful
	if( !uvShader.load("shaders/uv.vert", "shaders/uv.frag") ) {
        ofLogError("UV Remap-shader failed to load, exiting");
        ofExit();
	}
	if( !highlightShader.load("shaders/highlight.vert", "shaders/highlight.frag") ) {
        ofLogError("Highlight-shader failed to load, exiting");
        ofExit();
	}

	equirectangular.allocate(renderWidth, renderHeight, GL_RGB);

	camera = ofCamera();
	camera.setupPerspective(false, 90.);
	camera.setPosition(0,0,0);

	sphere = ofSpherePrimitive(200.,96);
	sphere.mapTexCoordsFromTexture(equirectangular.getTextureReference());
	sphere.setPosition(0,0,0);

	oculusRift.baseCamera = &camera;
	oculusRift.setup();
}


//--------------------------------------------------------------
void ofApp::update(){
	
	bool stitchVideoBuffer = false;

	ofVideoGrabber vidGrabber;
	ofImage videoImage;
	vector<ofVideoGrabber>::iterator grabberIt;
	vector<ofImage>::iterator imageIt;

	for(grabberIt = vidGrabbers.begin(), imageIt = videoImages.begin(); grabberIt != vidGrabbers.end(); ++grabberIt, ++imageIt) {
		vidGrabber = *grabberIt;

		vidGrabber.update();
		if (vidGrabber.isFrameNew()){
			int totalPixels = camWidth*camHeight*3;
			unsigned char * pixels = vidGrabber.getPixels();
			(*imageIt).setFromPixels(pixels, camWidth, camHeight, OF_IMAGE_COLOR);
			stitchVideoBuffer = true;
		}
	}

	if(stitchVideoBuffer) {
		equirectangular.begin();
		ofClear(32);
		ofEnableAlphaBlending();

		ofTexture mapTexture;
		ofTexture videoTexture;
		vector<ofTexture>::iterator mapIt;

		int width = equirectangular.getWidth();
		int height = equirectangular.getHeight();

		uvShader.begin();
		uvShader.setUniform2f("shaderSize", width, height);

		int i = 0;
		for(imageIt = videoImages.begin(), mapIt = mapTextures.begin(); imageIt != videoImages.end(); ++imageIt, ++mapIt, ++i) {
			videoTexture = (*imageIt).getTextureReference();
			mapTexture = (*mapIt);

			uvShader.setUniformTexture("map", mapTexture, 0);
			uvShader.setUniform2f("mapSize", mapTexture.getWidth(), mapTexture.getHeight());
			uvShader.setUniformTexture("frame", videoTexture, 1);
			uvShader.setUniform2f("frameSize", videoTexture.getWidth(), videoTexture.getHeight());
	
			//draw a fullscreen face for the texture
			glBegin(GL_QUADS);
			glTexCoord2f(width, height);         glVertex3f(0, 0, 0);
			glTexCoord2f(0, height);     glVertex3f(width, 0, 0);
			glTexCoord2f(0, 0); glVertex3f(width, height, 0);
			glTexCoord2f(width,0);     glVertex3f(0,height, 0);
			glEnd();
		}
		uvShader.end();

		equirectangular.end();
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofViewport(oculusViewport);
	if(oculusRift.isSetup()){
		glEnable(GL_DEPTH_TEST);
		equirectangular.getTextureReference().bind();

		oculusRift.beginLeftEye();
		sphere.draw();
		oculusRift.endLeftEye();
		
		oculusRift.beginRightEye();
		sphere.draw();
		oculusRift.endRightEye();
		
		oculusRift.draw();

		equirectangular.getTextureReference().unbind();		
		glDisable(GL_DEPTH_TEST);
	} else {
		camera.begin();
		equirectangular.getTextureReference().bind();

		sphere.draw();

		equirectangular.getTextureReference().unbind();
		camera.end();
	}
	
	ofViewport(previewViewport);
	highlightShader.begin();
	highlightShader.setUniform2f("frameSize", equirectangular.getWidth(), equirectangular.getHeight());
	highlightShader.setUniform1f("fov", ofDegToRad(camera.getFov()));
	highlightShader.setUniform3f("highlight", 1.,0.,0.);
	highlightShader.setUniform1f("linewidth", 0.02);
	ofVec3f dir = oculusRift.getOrientationQuat().getEuler();

	ofVec3f sphereDir = sphere.getOrientationEuler();
	highlightShader.setUniform2f("dir", ofDegToRad(dir.y-sphereDir.y), ofDegToRad(dir.x));

	float previewWidth = max(ofGetScreenWidth(), ofGetWidth());
	float previewHeight = previewViewport.width/2.;
	float previewBottom = (previewViewport.height + previewHeight)/2.;

	equirectangular.draw(previewWidth,previewBottom,-previewWidth, -previewHeight);
	highlightShader.end();
}


//--------------------------------------------------------------
void ofApp::keyPressed  (int key){ 
	/*
	if (key == 's') {
		vidGrabbers.at(0).videoSettings();
	}
	*/
	
	if( key == 'F' ) {
		ofSetWindowPosition(-1920,0);
		ofSetWindowShape(1920+1280,1080);
		oculusViewport = ofRectangle(0, 0, 1920,1080);
		previewViewport = ofRectangle(1920, 0, 1280, 800);
	}
	if( key == 'f' ) {
		ofToggleFullscreen();
	}	
	if (key == 'x') {
		int i = 0;
		ofImage videoImage;
		vector<ofImage>::iterator imageIt;

		for(imageIt = videoImages.begin(); imageIt != videoImages.end(); ++imageIt, ++i) {
			videoImage = *imageIt;
			string fileName = "grabbed_frame_"+ofToString(i)+".png";
			videoImage.saveImage(fileName);
		}
	}
	if( key == OF_KEY_LEFT ) {
		sphere.rotate(-1.,0.,1.,0.);
	}
	if( key == OF_KEY_RIGHT ) {
		sphere.rotate(1.,0.,1.,0.);
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){ 
	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
