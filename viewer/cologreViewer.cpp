#include <OIS/OIS.h>
#include "cologre.h"

using namespace std;

Ogre::Root*					ogreRoot				= NULL;
Ogre::SceneManager* sceneManager		= NULL;
Ogre::Camera*				camera					= NULL;
Ogre::Viewport*			viewport				= NULL;
Ogre::RenderWindow* window					= NULL;
OIS::Mouse*         oisms           = NULL;
OIS::Keyboard*      oiskb           = NULL;

int setupOgre();
void setupResources();
void moveCamera(float deltaT);

int main(int argc, char* argv[])
{
  setupOgre();
  setupResources();

  CColladaDatabase db;
  int ret = 0;
  if(ret = db.load("../../media/plane_box_light.dae"))
    exit(-1);
  CResourceConverter::m_convOptions.flipTextureH = true;
  db.convertResources();
  db.convertScene(sceneManager);

  //main loop
  while(!(oiskb->isKeyDown(OIS::KeyCode::KC_ESCAPE)))
  {
    static int frames;
    long timeMs = GetTickCount();
    static long second = timeMs;
    if(timeMs - second > 1000)
    { 
      cout << "FPS : " << frames << endl;
      frames = 0;
      second = timeMs;
    }

    moveCamera(0.05f);
    Ogre::WindowEventUtilities::messagePump();

		if(!ogreRoot->_fireFrameStarted()){
			return -1 ;
		}
	  window->update(true);

		if(!ogreRoot->_fireFrameEnded()){
			return -1 ;
		}
    frames++;
  }

	// Delete core system
	delete ogreRoot;
	return 0;
}

int setupOgre()
{
  ogreRoot = new Ogre::Root("./plugins.cfg", "./ogre.cfg");
  ogreRoot->showConfigDialog();
		
	Ogre::RenderSystem* rsystem = NULL;
	rsystem = ogreRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
	if(!rsystem)
		rsystem = ogreRoot->getRenderSystemByName("Direct3D9 Rendering Subsystem");
		
	if(!rsystem){
		std::cerr << "Could not find a rendersystem!\n";
		return -1;
	}
	ogreRoot->setRenderSystem(rsystem);

	ogreRoot->initialise(false);

	Ogre::NameValuePairList StartInfo;
	StartInfo.insert(Ogre::NameValuePairList::value_type("colourDepth", "24"));
	StartInfo.insert(Ogre::NameValuePairList::value_type("left", "100"));
	StartInfo.insert(Ogre::NameValuePairList::value_type("top", "100"));
	window = ogreRoot->createRenderWindow("cologreViewer", 800, 600, false, &StartInfo);
			
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;
	window->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

#if defined OIS_WIN32_PLATFORM
	pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
	pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
	pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
	pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
	pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
	pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
	pl.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
	pl.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif	

	OIS::InputManager* inputManager = OIS::InputManager::createInputSystem( pl );
  oiskb = static_cast<OIS::Keyboard*>(inputManager->createInputObject(OIS::OISKeyboard, true));	
  oisms = static_cast<OIS::Mouse*>(inputManager->createInputObject(OIS::OISMouse, true));

	//Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	// Now we can create a scene manager
  sceneManager = ogreRoot->createSceneManager(Ogre::ST_EXTERIOR_CLOSE, "sceneManager");
  sceneManager->setAmbientLight(Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f));

  // We need a camera too
  camera = sceneManager->createCamera("camera");
  camera->setPosition(Ogre::Vector3(0.0f, 5.0f, 20.0f));
  camera->lookAt(Ogre::Vector3(0.0f, 0.0f, 0.0f));
  camera->setNearClipDistance(0.05f);
  camera->setFarClipDistance(10000.0f);

	viewport = window->addViewport(camera);

  return 0;
}

/// Method which will define the source of resources (other than current folder)
void setupResources(void)
{
  // Load resource paths from config file
  Ogre::ConfigFile cf;
  cf.load("./resources.cfg");

  // Go through all sections & settings in the file
  Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

  Ogre::String secName, typeName, archName;
  while (seci.hasMoreElements())
  {
    secName = seci.peekNextKey();
    Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
    Ogre::ConfigFile::SettingsMultiMap::iterator i;
    for (i = settings->begin(); i != settings->end(); ++i)
    {
      typeName = i->first;
      archName = i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
      // OS X does not set the working directory relative to the app,
      // In order to make things portable on OS X we need to provide
      // the loading with it's own bundle path location
      ResourceGroupManager::getSingleton().addResourceLocation(String(macBundlePath() + "/" + archName), typeName, secName);
#else
      Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
#endif
    }
  }
  Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void moveCamera(float deltaT)
{
  oisms->capture();
  oiskb->capture();
  OIS::MouseState mouseState = oisms->getMouseState();

  mouseState.width = window->getWidth();
  mouseState.height = window->getHeight();

  if(oiskb->isKeyDown(OIS::KeyCode::KC_W))
    camera->moveRelative(Ogre::Vector3(0.0, 0.0, -50.0 * deltaT));
  else if(oiskb->isKeyDown(OIS::KeyCode::KC_S))
    camera->moveRelative(Ogre::Vector3(0.0, 0.0, 50.0 * deltaT));

  if(oiskb->isKeyDown(OIS::KeyCode::KC_A))
    camera->moveRelative(Ogre::Vector3(-50.0 * deltaT, 0.0, 0.0));
  else if(oiskb->isKeyDown(OIS::KeyCode::KC_D))
    camera->moveRelative(Ogre::Vector3(50.0 * deltaT, 0.0, 0.0));

  if(oiskb->isKeyDown(OIS::KeyCode::KC_PGUP))
    camera->moveRelative(Ogre::Vector3(0.0, 1.0 * deltaT, 0.0));
  else if(oiskb->isKeyDown(OIS::KeyCode::KC_PGDOWN))
    camera->moveRelative(Ogre::Vector3(0.0, -1.0 * deltaT, 0.0));

  camera->yaw(Ogre::Radian(Ogre::Degree(mouseState.X.rel)) * deltaT);
  camera->pitch(Ogre::Radian(Ogre::Degree(mouseState.Y.rel)) * deltaT);
}