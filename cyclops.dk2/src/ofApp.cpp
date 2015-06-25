#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	//ofSetLogLevel( OF_LOG_VERBOSE );
	ofBackground(20);

	oculusViewport = ofRectangle(0, 0, 800,800);
	previewViewport = ofRectangle(800, 0, 1280-800, 800);

			ofSetWindowPosition(-1920,0);
		ofSetWindowShape(1920+1280,1080);
		oculusViewport = ofRectangle(0, 0, 1920,1080);
		previewViewport = ofRectangle(1920, 0, 1280, 800);

		ofHideCursor();

	// create list of BlackMagic devices
	auto deviceList = ofxBlackmagic::Iterator::getDeviceList();

	remapVideoBuffer = false;

	for(auto device : deviceList) {
		auto input = shared_ptr<ofxBlackmagic::Input>(new ofxBlackmagic::Input());

		input->setUseTexture(true);

		auto mode = bmdModeHD1080i6000;
		input->startCapture(device, mode);
		this->inputs.push_back(input);
		mapTexture.allocate(1920, 1080, GL_RGB, true);
	}
	if(this->inputs.size() == 0) {
		// load still image if no blackmagic device is found
		ofLogNotice("No BlackMagic device found; using still image instead");
		ofPixels still;
		if( !ofLoadImage(still, "grabbed_frame.jpg") ) {
			ofLogError("Still image failed to load, exiting");
			ofExit();
		}

		//load the pixels to a texture
		videoTexture.allocate(still.getWidth(), still.getHeight(), GL_RGB, true);
		videoTexture.loadData(still);

		remapVideoBuffer = true;
	}

	// load highlight shader and exit if not succseful
	if( !highlightShader.load("shaders/highlight.vert", "shaders/highlight.frag") ) {
        ofLogError("Highlight Shader failed to load, exiting");
        ofExit();
	}

	// load uv shader and exit if not succseful
	if( !uvShader.load("shaders/uv.vert", "shaders/uv.frag") ) {
        ofLogError("UV Shader failed to load, exiting");
        ofExit();
	}

    //load mapfile pixels
    ofShortPixels pixels;
	if( !ofLoadImage(pixels, "maps/map.png")) {
        ofLogError("Map failed to load, exiting");
        ofExit();
	}

    //load the pixels to a texture
	mapTexture.allocate(pixels.getWidth(), pixels.getHeight(), GL_RGB16, true);
	mapTexture.loadData(pixels);

	equirectangular.allocate(pixels.getWidth(), pixels.getHeight(), GL_RGB);

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
	float width = mapTexture.getWidth(); 
	float height = mapTexture.getHeight();

	for(auto input : this->inputs) {
		input->update();
	}

	if(this->inputs.size() > 0) {
		auto input = inputs.at(0);
		videoTexture = input->getTextureReference();
		remapVideoBuffer = input->isFrameNew();
	}

	if (remapVideoBuffer) {
		equirectangular.begin();
		ofClear(0);

		uvShader.begin();
		uvShader.setUniformTexture("map", mapTexture, 0);
		uvShader.setUniform2f("mapSize", mapTexture.getWidth(), mapTexture.getHeight());
		uvShader.setUniformTexture("frame", videoTexture, 1);
		uvShader.setUniform2f("frameSize", videoTexture.getWidth(), videoTexture.getHeight());

		uvShader.setUniform2f("shaderSize", width, height);
	
		//draw a fullscreen face for the texture
		glBegin(GL_QUADS);
		glTexCoord2f(0, height);         glVertex3f(0, 0, 0);
		glTexCoord2f(width, height);     glVertex3f(width, 0, 0);
		glTexCoord2f(width, 0);          glVertex3f(width, height, 0);
		glTexCoord2f(0,0);               glVertex3f(0,height, 0);
		glEnd();
	
		uvShader.end();

		equirectangular.end();
		//remapVideoBuffer = false;
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
void ofApp::keyPressed(int key){
	if( key == 'F' ) {
		ofSetWindowPosition(-1920,0);
		ofSetWindowShape(1920+1280,1080);
		oculusViewport = ofRectangle(0, 0, 1920,1080);
		previewViewport = ofRectangle(1920, 0, 1280, 800);
	}
	if( key == 'f' ) {
		ofToggleFullscreen();
	}
	if( key == 'x' ) {
		string fileName = "grabbed_frame.png";

		ofImage videoImage;
		ofPixels pixels;
		pixels.allocate(videoTexture.getWidth(), videoTexture.getHeight(), OF_IMAGE_COLOR);
		videoTexture.readToPixels(pixels);
		videoImage.setFromPixels(pixels);
		videoImage.saveImage(fileName);
	}
	if( key == OF_KEY_LEFT ) {
		sphere.rotate(1.,0.,1.,0.);
	}
	if( key == OF_KEY_RIGHT ) {
		sphere.rotate(-1.,0.,1.,0.);
	}
	if(key == ' ') {
		auto deviceList = ofxBlackmagic::Iterator::getDeviceList();
		this->inputs.clear();
		for(auto device : deviceList) {
			auto input = shared_ptr<ofxBlackmagic::Input>(new ofxBlackmagic::Input());

			input->setUseTexture(true);

			auto mode = bmdModeHD1080i6000;
			input->startCapture(device, mode);
			this->inputs.push_back(input);
		}
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
