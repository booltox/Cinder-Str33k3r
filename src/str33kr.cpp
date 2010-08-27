#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderMath.h"
#include "cinder/Vector.h"
#include "cinder/Camera.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/Text.h"
#include "cinder/Area.h"
#include "cinder/Rect.h"
#include "Resources.h"

#define WIDTH 800
#define HEIGHT 600

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace gl;

class str33kr : public AppBasic {
public:
    void	prepareSettings( Settings *settings );
    void	setup();
    void	update();
    void	draw();
    void	keyDown( KeyEvent event );
    void	mouseMove( MouseEvent event );
    void	mouseDown( MouseEvent event );
    void	mouseDrag( MouseEvent event );
    void	mouseUp( MouseEvent event );
    void	resetFBOs();
    
    params::InterfaceGl	mParams1;
    
	int mCurrentFBO;
	int	mOtherFBO;
	gl::Fbo	mFBOs[2];
    gl::GlslProg mShriner;
	gl::Texture	mTexture;
    gl::Texture mBaseTex; // working image
	
	Vec2f mMouse;
	bool mMousePressed;
	
    float mThreshold;
	float mDecay;
    float mSaturation;
    
	static const int FBO_WIDTH  = WIDTH;
    static const int FBO_HEIGHT = HEIGHT;
    
private:
    vector<Capture>	mCaptures;
	vector<gl::Texture>	mTextures;
	vector<gl::Texture>	mNameTextures;
	vector<Surface>	mRetainedSurfaces;
    
};

void str33kr::prepareSettings( Settings *settings )
{
	settings->setWindowSize( FBO_WIDTH, FBO_HEIGHT );
	settings->setFrameRate( 60.0f );
}

void str33kr::mouseDown( MouseEvent event )
{
	mMousePressed = true;
}

void str33kr::mouseUp( MouseEvent event )
{
	mMousePressed = false;
}

void str33kr::mouseMove( MouseEvent event )
{
	mMouse = event.getPos();
}

void str33kr::mouseDrag( MouseEvent event )
{
	mMouse = event.getPos();
}

void str33kr::setup()
{
    // list out the devices
	vector<Capture::DeviceRef> devices( Capture::getDevices() );
	for( vector<Capture::DeviceRef>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
		Capture::DeviceRef device = *deviceIt;
		console() << "Found Device " << device->getName() << " ID: " << device->getUniqueId() << std::endl;
		try {
			if( device->checkAvailable() ) {
				mCaptures.push_back( Capture( FBO_WIDTH, FBO_HEIGHT, device ) );
				mCaptures.back().start();
                
				// placeholder text
				mTextures.push_back( gl::Texture() );
                
				// render the name as a texture
				TextLayout layout;
				layout.setFont( Font( "Arial", 24 ) );
				layout.setColor( Color( 1, 1, 1 ) );
				layout.addLine( device->getName() );
				mNameTextures.push_back( gl::Texture( layout.render( true ) ) );
			}
			console() << "device is NOT available" << std::endl;
		}
		catch( CaptureExc & ) {
			console() << "Unable to initialize device: " << device->getName() << endl;
		}
	}
    
	mMousePressed = false;
    
    mThreshold = 0.7f;
	mDecay = 0.002f;
    mSaturation = 12.0f;
        
    mParams1 = params::InterfaceGl( "Str33kr", Vec2i( 175, 100 ) );
	mParams1.addParam( "Threshold  T", &mThreshold, "min=0.0 max=1.0 step=0.05 keyIncr=T keyDecr=t" );
	mParams1.addParam( "DecayRate  D", &mDecay, "min=0.0 max=0.02 step=0.001 keyIncr=D keyDecr=d" );
	mParams1.addParam( "Saturation S", &mSaturation, "min=0.0 max=100.0 step=1.0 keyIncr=S keyDecr=s" );	
	gl::Fbo::Format format;
	format.enableDepthBuffer( false );
	
	mCurrentFBO = 0;
	mOtherFBO = 1;
	mFBOs[0] = gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mFBOs[1] = gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
    
    mShriner = gl::GlslProg( loadResource( RES_PASS_THRU_VERT ), loadResource( RES_SHRINER_FRAG ) );
	mTexture = gl::Texture( loadImage( loadResource( RES_STARTER_IMAGE ) ) );
	mTexture.setWrap( GL_REPEAT, GL_REPEAT );
	mTexture.setMinFilter( GL_LINEAR );
	mTexture.setMagFilter( GL_LINEAR );
    
    mBaseTex = gl::Texture( FBO_WIDTH, FBO_HEIGHT );
    mBaseTex.setWrap( GL_REPEAT, GL_REPEAT );
	mBaseTex.setMinFilter( GL_LINEAR );
	mBaseTex.setMagFilter( GL_LINEAR );
    mBaseTex.bind( 2 );
	//mTexture.bind( 2 );
	
	resetFBOs();
}

void str33kr::update()
{	
    for( vector<Capture>::iterator cIt = mCaptures.begin(); cIt != mCaptures.end(); ++cIt ) {
		if( cIt->checkNewFrame() ) {
			Surface8u surf = cIt->getSurface();
			mTextures[cIt - mCaptures.begin()] = gl::Texture( surf );
		}
	}
    
    if( mTextures[0] ){
        mTexture = mTextures[0];//gl::Texture( loadImage( loadResource( RES_STARTER_IMAGE ) ) );
        mTexture.setWrap( GL_REPEAT, GL_REPEAT );
        mTexture.setMinFilter( GL_LINEAR );
        mTexture.setMagFilter( GL_LINEAR );
        mTexture.bind( 0 );
    }
    
	// normally setMatricesWindow flips the projection vertically so that the upper left corner is 0,0
	// but we don't want to do that when we are rendering the FBOs onto each other, so the last param is false
	gl::setMatricesWindow( mFBOs[0].getSize(), false );
	gl::setViewport( mFBOs[0].getBounds() );
       
    // just draw a quad
    mFBOs[ 0 ].bindFramebuffer();
    
    mShriner.bind();
    mShriner.uniform( "basetex", 2 );
    mShriner.uniform( "lighttex", 0 );
    mShriner.uniform( "threshold", mThreshold );
    mShriner.uniform( "decayrate", mDecay );
    mShriner.uniform( "saturation", mSaturation );
    mShriner.uniform( "fakecolor", Vec4f( 0.f, 0.f, 0.f, 0.f ), 1 );
    
    //gl::clear( ColorA( 0, 0.f, 0, 0 ) );
    gl::drawSolidRect( mFBOs[ 0 ].getBounds(), false );
    
    mShriner.unbind();
    
    mFBOs[0].unbindFramebuffer();
    
    mCurrentFBO = 0;
    mFBOs[0].bindTexture( 2 );
}


void str33kr::draw()
{
	//gl::clear( ColorA( 0, 0, 0, 0 ) );
	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );
    
    gl::draw( mTexture, getWindowBounds() );
    
    gl::enableAlphaBlending();
    gl::enableAdditiveBlending();
    gl::draw( mFBOs[mCurrentFBO].getTexture(), getWindowBounds() );
    gl::disableAlphaBlending();
	
	params::InterfaceGl::draw();
}

void str33kr::resetFBOs()
{
	mTexture.bind( 0 );
	gl::setMatricesWindow( mFBOs[0].getSize(), false );
	gl::setViewport( mFBOs[0].getBounds() );
	for( int i = 0; i < 2; i++ ){
		mFBOs[i].bindFramebuffer();
		gl::draw( mTexture, mFBOs[i].getBounds() );
	}
	gl::Fbo::unbindFramebuffer();
}

void str33kr::keyDown( KeyEvent event )
{
	if( event.getChar() == 'r' ) {
		resetFBOs();
	}
    
    if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
	else if( event.getChar() == ' ' ) {
		mCaptures.back().isCapturing() ? mCaptures.back().stop() : mCaptures.back().start();
	}
	else if( event.getChar() == 'y' ) {
		// retain a random surface to exercise the surface caching code
		int device = rand() % ( mCaptures.size() );
		mRetainedSurfaces.push_back( mCaptures[device].getSurface() );
		console() << mRetainedSurfaces.size() << " surfaces retained." << std::endl;
	}
	else if( event.getChar() == 'u' ) {
		// unretain retained surface to exercise the Capture's surface caching code
		if( ! mRetainedSurfaces.empty() )
			mRetainedSurfaces.pop_back();
		console() << mRetainedSurfaces.size() << " surfaces retained." << std::endl;
	}
    
}

CINDER_APP_BASIC( str33kr, RendererGl )
