#include <HellEngine.h>
#include "imgui/imgui.h"
#include <cmath> 
#include "Platform/OpenGL/glExtension.h" // glInfo struct
#include "Config.h"
#include "HellEngine/Animation/SkinnedMesh.h"
#include "HellEngine/Audio/Audio.h"
#include "HellEngine/GameObjects/Decal.h"
#include "HellEngine/GameObjects/Shell.h"

namespace HellEngine
{
	//bool Config::DOF_showFocus;
	bool Config::DOF_vignetting;
	float Config::DOF_vignout;
	float Config::DOF_vignin;
	float Config::DOF_vignfade;
	float Config::DOF_CoC;
	float Config::DOF_maxblur;
	int Config::DOF_samples;
	int Config::DOF_rings;
	float Config::DOF_threshold;
	float Config::DOF_gain;
	float Config::DOF_bias;
	float Config::DOF_fringe;

	class ExampleLayer : public HellEngine::Layer
	{
	public:

		const int MAX_LIGHTS = 8;

		int SCR_WIDTH, SCR_HEIGHT;
		bool _IMGUI_RUN_ONCE = true;

		bool showBoundingBoxes = false;
		bool showBuffers = false;
		bool showVolumes = false;
		bool showLights = false;
		bool showImGUI = true;
		bool optimise = false;
		bool NoClip = false;
		bool showRaycastPlane = false;

		glm::vec3 ray_direction;
		RaycastData raycastData;

		bool shellEjected = false;
		bool shotgunFiring = false;

		Camera camera;
		float deltaTime;
		float lastFrame;

		File file;

		vector<MousePickInfo> mousePickIndices;
		MousePickInfo mousePicked{ MousePickType::NotFound, -1 };

		House house;
		Player player;

		bool runOnce = true;

		Skybox skybox;

		Cube cube;
		Plane plane;
		Plane plane2;
		Cube lightCube;

		PBO pBuffer;
		GBuffer gBuffer;
		LightingBuffer lightingBuffer;
		std::vector<BlurBuffer> blurBuffers;
		ShadowCubeMapArray shadowCubeMapArray;

		float roughness = 0.077f;
		float metallic = 0.546f;
		float brightness = 0;
		float contrast = 1.021f;

		float bias = 0.09f;

		float lightRadius = 3.0f;;
		float lightCompression = 3.0f;

		int selectedWeapon = 1;

		unsigned int skyboxVAO;

		vector<BoundingBox*> boundingBoxPtrs;
		vector<BoundingPlane*> boundingPlanePtrs; 

		vector<Decal> decals;
		vector<Shell> shells;


		Model* bebop;

		float time = 0;

		glm::vec3 WorkBenchPosition;

		SkinnedMesh skinnedMesh;

		ExampleLayer() : Layer("Example")
		{
			Audio::LoadAudio("Door1.wav");
			Audio::LoadAudio("Shotgun.wav");
			Audio::LoadAudio("ShellBounce.wav");
		//	Audio::LoadAudio("Music.mp3");
			//Audio::LoadAudio("Music2.mp3");
			//Audio::LoadAudio("Music3.mp3");
			Audio::LoadAudio("player_step_1.wav");
			Audio::LoadAudio("player_step_2.wav");
			Audio::LoadAudio("player_step_3.wav");
			Audio::LoadAudio("player_step_4.wav");


			//ozz = Ozz_Animation("res/objects/seymour_skeleton.ozz"); 

			deltaTime = 0.0f;
			lastFrame = 0.0f;

			WorkBenchPosition = glm::vec3(5.25f, 0, 2.8f);

			// App setup
			Application& app = Application::Get();
			SCR_WIDTH = app.GetWindow().GetWidth();
			SCR_HEIGHT = app.GetWindow().GetHeight();

			GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());


			// pbo
			pBuffer = PBO(SCR_WIDTH, SCR_HEIGHT);

			// setup
			Config::Load();

			//Audio audio;

			// MODELS: load files
			ModelLoader loader;
			//Model::models.push_back(loader.LoadFromFile("AnimatedShotgun.FBX"));
			//Model::models.push_back(loader.LoadFromFile("AnimatedShotgun.dae"));
			//Model::models.push_back(loader.LoadFromFile("model.dae"));
			//Model::models.push_back(loader.LoadFromFile("Shotgun.dae"));
			Model::models.push_back(loader.LoadFromFile("Wall.obj"));
			Model::models.push_back(loader.LoadFromFile("Door.obj"));
			Model::models.push_back(loader.LoadFromFile("DoorShadowCaster.obj"));
			Model::models.push_back(loader.LoadFromFile("Door_jam.obj"));
			Model::models.push_back(loader.LoadFromFile("Wall_DoorHole.obj"));
			Model::models.push_back(loader.LoadFromFile("UnitPlane.obj"));
			Model::models.push_back(loader.LoadFromFile("Light.obj"));
			Model::models.push_back(loader.LoadFromFile("SphereLight.obj"));
			Model::models.push_back(loader.LoadFromFile("sphere.obj"));
			Model::models.push_back(loader.LoadFromFile("Shell.fbx"));
			Model::models.push_back(loader.LoadFromFile("Old_Cotton_Couch.obj"));
			Model::models.push_back(loader.LoadFromFile("PictureFrame.FBX"));

			//Model::models.push_back(loader.LoadFromFile("DoorFinal.obj"));

			Material::BuildMaterialList();

			// Temporary commented out to speed up load time
			if (true) {
				Texture::Init();

				//


				//Model::models.push_back(loader.LoadFromFile("Mannequin.obj"));
				/*
				Model::models.push_back(loader.LoadFromFile("Shotgun.obj"));
				Model::models.push_back(loader.LoadFromFile("Shotgun2.obj"));
				Model::models.push_back(loader.LoadFromFile("knight.dae"));
				Model::models.push_back(loader.LoadFromFile("Colt45.obj"));
				Model::models.push_back(loader.LoadFromFile("Bench.obj"));
				Model::models.push_back(loader.LoadFromFile("SM_Shotgun_01a.FBX"));

				Model::models.push_back(loader.LoadFromFile("Shotgun3.obj"));
				Model::models.push_back(loader.LoadFromFile("Handgun.obj"));
				Model::models.push_back(loader.LoadFromFile("REDoor.obj"));
				*/
			}



			// MODELS: set pointers
			Wall::model = Model::GetByName("Wall.obj");
			//Door::modelDoor = Model::GetByName("DoorFinal.obj");
			Door::modelDoor = Model::GetByName("Door.obj");
			Door::modelDoorShadowCaster = Model::GetByName("DoorShadowCaster.obj");
			Door::modelDoorJam = Model::GetByName("Door_jam.obj");
			Door::modelWallHole = Model::GetByName("Wall_DoorHole.obj");

			File::LoadMapNames();

			//house.LoadTestScene();
			house = File::LoadMap("Level2.map");
			house.RebuildRooms();
			RebuildMap();


			// OPENGL
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
			const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

			std::cout << "VENDOR: " << vendor << "\n";
			std::cout << "RENDERER: " << renderer << "\n";

			// Check GL extensions
			glExtension& ext = glExtension::getInstance();
			if (ext.isSupported("GL_ARB_pixel_buffer_object"))
				std::cout << "Video card supports GL_ARB_pixel_buffer_object.\n";
			else
				std::cout << "[ERROR] Video card does not supports GL_ARB_pixel_buffer_object.\n";

			// Framebuffers
			shadowCubeMapArray = ShadowCubeMapArray(MAX_LIGHTS);
			gBuffer = GBuffer(SCR_WIDTH, SCR_HEIGHT);
			lightingBuffer = LightingBuffer(SCR_WIDTH, SCR_HEIGHT);

			blurBuffers.push_back(BlurBuffer(SCR_WIDTH / 2, SCR_HEIGHT / 2));
			blurBuffers.push_back(BlurBuffer(SCR_WIDTH / 4, SCR_HEIGHT / 4));
			blurBuffers.push_back(BlurBuffer(SCR_WIDTH / 8, SCR_HEIGHT / 8));
			blurBuffers.push_back(BlurBuffer(SCR_WIDTH / 16, SCR_HEIGHT / 16));

			// OpenGL
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glPointSize(6);

			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			// Camera
			camera.Position = glm::vec3(0.6f, 0.0f, 9.45f);
			camera.CalculateProjectionMatrix(SCR_WIDTH, SCR_HEIGHT);

			// Shaders
			Shader::LoadShader("LampShader", "white.vert", "white.frag", "NONE");
			Shader::LoadShader("debugFboShader", "fbo_debug.vert", "fbo_debug.frag", "NONE");
			Shader::LoadShader("Geometry", "geometry.vert", "geometry.frag", "NONE");
			Shader::LoadShader("Shadow", "shadow.vert", "shadow.frag", "NONE");
			Shader::LoadShader("SimpleDepth", "shadowDepth.vert", "shadowDepth.frag", "shadowDepth.geom");
			Shader::LoadShader("Skybox", "skybox.vert", "skybox.frag", "NONE");
			Shader::LoadShader("Lighting", "lighting.vert", "lighting.frag", "NONE");
			Shader::LoadShader("DEBUG", "test.vert", "test.frag", "NONE");
			Shader::LoadShader("Composite", "composite.vert", "composite.frag", "NONE");
			Shader::LoadShader("NullTechnique", "null_technique.vert", "null_technique.frag", "NONE");
			Shader::LoadShader("BlurHorizontal", "blurHorizontal.vert", "blur.frag", "NONE");
			Shader::LoadShader("BlurVertical", "blurVertical.vert", "blur.frag", "NONE");
			Shader::LoadShader("Final", "final.vert", "final.frag", "NONE");
			Shader::LoadShader("DownScale", "downScale.vert", "downScale.frag", "NONE");
			Shader::LoadShader("Final", "final.vert", "final.frag", "NONE");
			Shader::LoadShader("DOF", "DOF.vert", "DOF.frag", "NONE");
			Shader::LoadShader("Decals", "decals.vert", "decals.frag", "NONE");
			//Shader::LoadShader("Fxaa", "debugFboShader.vert", "Fxaa.frag", "NONE");

			Shader* lampShader = Shader::GetShaderByName("LampShader");
			Shader* shadowShader = Shader::GetShaderByName("Shadow");
			Shader* simpleDepthShader = Shader::GetShaderByName("SimpleDepth");

			Shader* geometryShader = Shader::GetShaderByName("Geometry");
			geometryShader->use();
			geometryShader->setInt("diffuseTexture", 0);
			geometryShader->setInt("roughnessTexture", 1);
			geometryShader->setInt("metallicTexture", 2);
			geometryShader->setInt("normalMap", 3);
			geometryShader->setInt("emissiveMap", 4);
			geometryShader->setMat4("projection", camera.projectionMatrix);

			Shader* decalShader = Shader::GetShaderByName("Decals");
			decalShader->use();
			decalShader->setInt("depthTexture", 0);
			decalShader->setInt("diffuseTexture", 1);
			decalShader->setInt("normalTexture", 2);


			//for (int i = 0; i < 50; i++)
			//	geometryShader->setMat4("jointTransforms[" + std::to_string(i) + "]", glm::mat4(1));


			shadowShader->use();
			shadowShader->setInt("diffuseTexture", 0);
			shadowShader->setInt("depthMap", 1);
			//simpleDepthShader.setInt("lightIndex", 1);


			//glUseProgram(lampShader->ID);
			lampShader->use();
			lampShader->setMat4("projection", camera.projectionMatrix);

			Shader* DEBUGShader = Shader::GetShaderByName("DEBUG");
			DEBUGShader->use();
			DEBUGShader->setMat4("projection", camera.projectionMatrix);

			Shader* skyboxShader = Shader::GetShaderByName("Skybox");
			skyboxShader->use();
			skyboxShader->setInt("cubeMap", 0);
			skyboxShader->setMat4("projection", camera.projectionMatrix);


			Shader* lightingShader = Shader::GetShaderByName("Lighting");
			lightingShader->use();
			lightingShader->setInt("albedoTexture", 0);
			lightingShader->setInt("normalTexture", 1);
			lightingShader->setInt("depthTexture", 2);
			lightingShader->setInt("emissiveTexture", 3);
			lightingShader->setInt("shadowMaps", 4);

			Shader* compositeShader = Shader::GetShaderByName("Composite");
			compositeShader->use();
			compositeShader->setInt("albedoTexture", 0);
			compositeShader->setInt("accumulatedLighting", 1);

			Shader* blurHorizontalShader = Shader::GetShaderByName("BlurHorizontal");
			blurHorizontalShader->use();
			blurHorizontalShader->setInt("image", 0);

			Shader* blurVerticalShader = Shader::GetShaderByName("BlurVertical");
			blurVerticalShader->use();
			blurVerticalShader->setInt("image", 0);

			Shader* downScaleShader = Shader::GetShaderByName("DownScale");
			downScaleShader->use();
			downScaleShader->setInt("inputTexture", 0);

			Shader* DOFShader = Shader::GetShaderByName("DOF");
			DOFShader->use();
			DOFShader->setInt("renderTexture", 0);
			DOFShader->setInt("depthTexture", 1);

			Shader* finalShader = Shader::GetShaderByName("Final");
			finalShader->use();
			finalShader->setInt("scene", 0);
			finalShader->setInt("blur0", 1);
			finalShader->setInt("blur1", 2);
			finalShader->setInt("blur2", 3);
			finalShader->setInt("blur3", 4);




			glm::vec3 couchPosition = glm::vec3(-2.9f, 0, 6.1f);
			RenderableObject::NewObject("Couch", Model::GetByName("Old_Cotton_Couch.obj"));
			RenderableObject::SetPositionByName("Couch", couchPosition);
			RenderableObject::SetScaleByName("Couch", glm::vec3(0.07));
			//RenderableObject::SetAngleByName("Couch", PI);
			RenderableObject::SetRotationByName("Couch", glm::vec3(0, PI, 0));
			RenderableObject::SetDiffuseTextureByName("Couch", Texture::GetIDByName("Couch_A_Base_Color.png"));
			RenderableObject::SetRoughnessTextureByName("Couch", Texture::GetIDByName("Couch_A_Roughness.png"));
			RenderableObject::SetMetallicTextureByName("Couch", Texture::GetIDByName("Couch_A_Metallic.png"));
			RenderableObject::SetNormalMapByName("Couch", Texture::GetIDByName("Couch_A_NormalMap.png"));
			BoundingBox bb = BoundingBox(couchPosition, glm::vec3(2.1f, 0.8f, 0.9f), BOTTOM_CENTERED);
			RenderableObject::SetBoundingBoxByName("Couch", bb);




			glm::vec3 framePosition = glm::vec3(0, 1.6f, 3.25f);
			RenderableObject::NewObject("PictureFrame", Model::GetByName("PictureFrame.FBX"));
			RenderableObject::SetPositionByName("PictureFrame", framePosition);
			RenderableObject::SetScaleByName("PictureFrame", glm::vec3(0.9));
			RenderableObject::SetRotationByName("PictureFrame", glm::vec3(ROTATE_270, ROTATE_90, 0));
			RenderableObject::SetDiffuseTextureByName("PictureFrame", Texture::GetIDByName("PictureFrame_BaseColor.png"));
			RenderableObject::SetRoughnessTextureByName("PictureFrame", Texture::GetIDByName("PictureFrame_Roughness.png"));
			RenderableObject::SetMetallicTextureByName("PictureFrame", Texture::GetIDByName("PictureFrame_Metallic.png"));
			RenderableObject::SetNormalMapByName("PictureFrame", Texture::GetIDByName("PictureFrame_NormalMap.png"));
			BoundingBox bb2 = BoundingBox(framePosition, glm::vec3(0.05, 0.5f, 0.08f), ObjectOrigin::BOTTOM_CENTERED);
			RenderableObject::SetBoundingBoxByName("PictureFrame", bb2);




			//auto loader = ModelLoader();
			//bebop = loader.LoadFromFile("COWBOY", "model.dae");



			//ModelLoader::LoadModel("Door.obj");

			
		


			if (false) {
				//RenderableObject::NewObject("Cowboy", Model::GetByName("Shotgun.dae"));
				RenderableObject::NewObject("Cowboy", Model::GetByName("model.dae"));
				//RenderableObject::NewObject("Cowboy", Model::GetByName("AnimatedShotgun.FBX")); 
				RenderableObject::SetPositionByName("Cowboy", glm::vec3(0, 0, 1.10f));
				RenderableObject::SetScaleByName("Cowboy", glm::vec3(0.2));
				//RenderableObject::SetRotateAngleByName("Cowboy", glm::vec3(1, 0, 0));
				//RenderableObject::SetAngleByName("Cowboy", PI * 1.5f);
				RenderableObject::SetDiffuseTextureByName("Cowboy", Texture::GetIDByName("eye.png"));
			}



			/*if (false) {
				RenderableObject::NewObject("Cowboy", Model::GetByName("model.dae"));
				//RenderableObject::NewObject("Cowboy", Model::GetByName("AnimatedShotgun.dae"));
				RenderableObject::SetPositionByName("Cowboy", glm::vec3(0, 0, 1.00f));
				RenderableObject::SetScaleByName("Cowboy", glm::vec3(0.01));
				RenderableObject::SetDiffuseTextureByName("Cowboy", Texture::GetIDByName("eye.png"));
			}*/


			//RenderableObject::SetScaleByName("Cowboy", glm::vec3(1));
			//RenderableObject::SetAngleByName("Cowboy", 0);
			//RenderableObject::SetPositionByName("Cowboy", glm::vec3(0, 0, 0));

			/*RenderableObject::NewObject("Shepherd", Model::GetByName("Shepherd.fbx"));
			RenderableObject::NewObject("Shepherd", Model::GetByName("Old_Cotton_Couch.obj"));
			RenderableObject::SetPositionByName("Shepherd", glm::vec3(0, 0, 3.5f));
			RenderableObject::SetScaleByName("Shepherd", glm::vec3(0.01));
			RenderableObject::SetAngleByName("Shepherd", PI);
			RenderableObject::SetDiffuseTextureByName("Shepherd", Texture::GetIDByName("Couch_A_Base_Color.png"));
			RenderableObject::SetRoughnessTextureByName("Shepherd", Texture::GetIDByName("Couch_A_Roughness.png"));
			RenderableObject::SetMetallicTextureByName("Shepherd", Texture::GetIDByName("Couch_A_Metallic.png"));*/


			if (false)
			{

				RenderableObject::NewObject("REDoor", Model::GetByName("REDoor.obj"));
				RenderableObject::SetPositionByName("REDoor", glm::vec3(3, 0, 2));
				RenderableObject::SetScaleByName("REDoor", glm::vec3(0.9f));
				//RenderableObject::SetRotateAngleByName("REDoor", glm::vec3(1, 0, 0));
				//RenderableObject::SetAngleByName("REDoor", PI * 1.5f);
				RenderableObject::SetDiffuseTextureByName("REDoor", Texture::GetIDByName("REDoor_BaseColor.png"));
				RenderableObject::SetRoughnessTextureByName("REDoor", Texture::GetIDByName("REDoor_Roughness.png"));
				RenderableObject::SetMetallicTextureByName("REDoor", Texture::GetIDByName("REDoor_Metallic.png"));
				RenderableObject::SetNormalMapByName("REDoor", Texture::GetIDByName("REDoor_NormalMap.png"));



				RenderableObject::NewObject("Bench", Model::GetByName("Bench.obj"));
				RenderableObject::SetPositionByName("Bench", glm::vec3(5.25f, 0, 2.8f));
				//RenderableObject::SetAngleByName("Bench", PI * 1.5f);
				RenderableObject::SetDiffuseTextureByName("Bench", Texture::GetIDByName("Workbench_Base_Color.png"));
				RenderableObject::SetRoughnessTextureByName("Bench", Texture::GetIDByName("EmptyMetallic.png"));
				RenderableObject::SetMetallicTextureByName("Bench", Texture::GetIDByName("Workbench_Metallic.png"));
				RenderableObject::SetNormalMapByName("Bench", Texture::GetIDByName("Workbench_NormalMap.png"));
				BoundingBox bb2 = BoundingBox(WorkBenchPosition, glm::vec3(0.5f, 0.98f, 2.2f), BOTTOM_CENTERED);
				RenderableObject::SetBoundingBoxByName("Bench", bb2);

				RenderableObject::NewObject("Shotty2", Model::GetByName("Shotgun3.obj"));
				RenderableObject::SetPositionByName("Shotty2", glm::vec3(5.20f, 0, 2.8f));
				//RenderableObject::SetAngleByName("Shotty2", PI * 1.5f);
				RenderableObject::SetDiffuseTextureByName("Shotty2", Texture::GetIDByName("Shotgun2_BaseColor.png"));
				RenderableObject::SetRoughnessTextureByName("Shotty2", Texture::GetIDByName("Shotgun2_Roughness.png"));
				RenderableObject::SetMetallicTextureByName("Shotty2", Texture::GetIDByName("Shotgun2_Metallic.png"));
				RenderableObject::SetNormalMapByName("Shotty2", Texture::GetIDByName("Shotgun2_NormalMap.png"));

				/*	RenderableObject::NewObject("Handgun", Model::GetByName("Handgun.obj"));
					RenderableObject::SetPositionByName("Handgun", glm::vec3(5.25f, 0, 2.8f));
					RenderableObject::SetAngleByName("Handgun", PI * 1.5f);
					RenderableObject::SetDiffuseTextureByName("Handgun", Texture::GetIDByName("M1911_BaseColor.png"));
					RenderableObject::SetRoughnessTextureByName("Handgun", Texture::GetIDByName("M1911_Roughness.png"));
					RenderableObject::SetMetallicTextureByName("Handgun", Texture::GetIDByName("M1911_Metallic.png"));
					RenderableObject::SetNormalMapByName("Handgun", Texture::GetIDByName("EmptyNormalMap.png"));*/

					/*float mannequinAngle = 2.0f;
					glm::vec3 mannequinPosition = glm::vec3(-1, 0, 2.0f);
					RenderableObject::NewObject("Mannequin", Model::GetByName("Mannequin.obj"));
					RenderableObject::SetPositionByName("Mannequin", mannequinPosition);
					RenderableObject::SetAngleByName("Mannequin", mannequinAngle);
					RenderableObject::SetDiffuseTextureByName("Mannequin", Texture::GetIDByName("Plastic2.png"));
					RenderableObject::SetRoughnessTextureByName("Mannequin", Texture::GetIDByName("Plastic2.png"));
					RenderableObject::SetMetallicTextureByName("Mannequin", Texture::GetIDByName("Plastic2.png"));
					BoundingBox bb2 = BoundingBox(mannequinPosition, glm::vec3(0.3f, 1.8f, 0.3f), BOTTOM_CENTERED);
					bb2.SetAngle(mannequinAngle);
					RenderableObject::SetBoundingBoxByName("Mannequin", bb2);
					*/

				/*RenderableObject::NewObject("Weapon", Model::GetByName("SM_Shotgun_01a.FBX"));
				RenderableObject::SetPositionByName("Weapon", glm::vec3(0, 1.2f, 2.2f));
				RenderableObject::SetScaleByName("Weapon", glm::vec3(0.2f));
				RenderableObject::SetAngleByName("Weapon", ROTATE_90);
				RenderableObject::SetDiffuseTextureByName("Weapon", Texture::GetIDByName("Shotgun2_BaseColor.png"));
				RenderableObject::SetRoughnessTextureByName("Weapon", Texture::GetIDByName("Shotgun2_Roughness.png"));
				RenderableObject::SetMetallicTextureByName("Weapon", Texture::GetIDByName("Shotgun2_Metallic.png"));
				RenderableObject::SetNormalMapByName("Weapon", Texture::GetIDByName("Shotgun2_NormalMap.png"));*/

			

				//	RenderableObject::NewObject("Weapon", Model::GetByName("Colt45.obj"));

					/*RenderableObject::NewObject("Shotgun", Model::GetByName("Shotgun.obj"));
					RenderableObject::SetPositionByName("Shotgun", glm::vec3(0, 1, 2.2f));
					RenderableObject::SetScaleByName("Shotgun", glm::vec3(1));`
					RenderableObject::SetAngleByName("Shotgun", 0);
					RenderableObject::SetDiffuseTextureByName("Shotgun", Texture::GetIDByName("Shotgun_BaseColor.png"));
					RenderableObject::SetRoughnessTextureByName("Shotgun", Texture::GetIDByName("Shotgun_Roughness.png"));
					RenderableObject::SetMetallicTextureByName("Shotgun", Texture::GetIDByName("Shotgun_Metallic.png"));
					RenderableObject::SetNormalMapByName("Shotgun", Texture::GetIDByName("Shotgun_NormalMap.png"));*/


					//shader->setFloat("texScale", 10.0f);
					//shader->setFloat("roughness", 0.1f);
					//shader->setFloat("metallic", 0.1f);
					//RenderableObject m = RenderableObject(Model::GetByName("Mannequin.obj"), "Plastic2.png", mannequinPosition,2.0f, glm::vec3(0.9f));
					//m.Draw(shader, bindTextures);
					//shader->setFloat("texScale", 1);

					/*RenderableObject::NewObject("Sphere", Model::GetByName("sphere.obj"));
					RenderableObject::SetScaleByName("Sphere", glm::vec3(0.1));
					RenderableObject::SetDiffuseTextureByName("Sphere", Texture::GetIDByName("Couch_A_Roughness.png"));
					RenderableObject::SetMetallicTextureByName("Sphere", Texture::GetIDByName("Couch_A_Roughness.png"));
					RenderableObject::SetRoughnessTextureByName("Sphere", Texture::GetIDByName("Couch_A_Roughness.png"));*/

					/*RenderableObject::NewObject("Shells", Model::GetByName("Shells.obj"));
					RenderableObject::SetPositionByName("Shells", glm::vec3(0, 0, 0));
					RenderableObject::SetScaleByName("Shells", glm::vec3(0.01f));
					RenderableObject::SetAngleByName("Shells", PI);
					RenderableObject::SetDiffuseTextureByName("Shells", Texture::GetIDByName("Couch_A_Base_Color.png"));
					RenderableObject::SetRoughnessTextureByName("Shells", Texture::GetIDByName("Couch_A_Roughness.png"));
					RenderableObject::SetMetallicTextureByName("Shells", Texture::GetIDByName("Couch_A_Metallic.png"));*/
			}

			// My objects
			skybox = Skybox(glm::vec3(0));
			/*cube = Cube(-0.75f, 0.5f, 3.25f);
			plane = Plane(0, 2.4f, 0, glm::vec3(20));
			lightCube = Cube(0, 0, 0, glm::vec3(0.1));

			cube.angle = -0.25f;
			cube.scale = glm::vec3(0.8);*/

			// Setup house
			RebuildMap();

			std::cout << "\n";

			/////////////////////////////////////
			// LOAD THE MOTHER FUCKING SHOTGUN //
			/////////////////////////////////////

			if (!skinnedMesh.LoadMesh("res/objects/AnimatedShotgun.FBX"))
				HELL_ERROR("Mesh load failed\n");
			else
				HELL_ERROR("Mesh loaded!\n");

			// Begin music
			Audio::StreamAudio("Music.mp3");
		}

		vector<BoundingBox*> CreateBoudingBoxPtrsVector()
		{
			vector<BoundingBox*> b;

			for (Door & d : house.doors)
				b.push_back(&d.boundingBox);
			for (RenderableObject & r : RenderableObject::renderableObjects)
				if (r.hasCollision)
					b.push_back(&r.boundingBox);

			return b;
		}

		vector<BoundingPlane*> CreateBoudingPlanePtrsVector()
		{
			vector<BoundingPlane*> planes;
			
			// Iterate over ever room in the house, add the walls
			for (Room& room : house.rooms)
			{
				for (Wall& w : room.walls)
					planes.push_back(&w.boundingPlane);
				planes.push_back(&room.floor.plane);
				planes.push_back(&room.ceiling.plane);
			}
			HELL_ERROR("planes: " + std::to_string(planes.size()));

			for (BoundingBox* box : boundingBoxPtrs)
			{
				BoundingPlane* p0 = &box->frontPlane;
				BoundingPlane* p1 = &box->backPlane;
				BoundingPlane* p2 = &box->leftPlane;
				BoundingPlane* p3 = &box->rightPlane;

				planes.push_back(p0);
				planes.push_back(p1);
				planes.push_back(p2);
				planes.push_back(p3);
			}

			return planes;
		}

		float RandomFloat(float a, float b) {
			float random = ((float)rand()) / (float)RAND_MAX;
			float diff = b - a;
			float r = random * diff;
			return a + r;
		}

		float test = 0;



		void OnUpdate() override
		{
			Application& app = Application::Get();
			GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
			glfwGetWindowSize(window, &SCR_WIDTH, &SCR_HEIGHT);
			app.GetWindow().SetVSync(true);

			float currentFrame = (float)glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			camera.Update(deltaTime);
			skinnedMesh.Update(deltaTime, &camera);

			Audio::MainUpdate();
			
			time += deltaTime;

			if (player.walking)
				skinnedMesh.headBobCounter += deltaTime * skinnedMesh.headBobSpeed;

			// GAME OBJECTS
			for (Shell& shell : shells)
				shell.Update(deltaTime);

			raycastData = GetRaycastData(0);
		

			//ShotgunLogic::Update(deltaTime, &shells, &skinnedMesh, &camera);

			
			if (!shellEjected && time > 0.45f)
			{
				shellEjected = true;
				shells.push_back(Shell(Model::GetByName("Shell.fbx"), skinnedMesh.boltPos + (camera.Up * -Shell::shellUpFactor) + (camera.Right * Shell::shellRightFactor) + (camera.Front * -Shell::shellForwardFactor), &camera, deltaTime));
			}

			if (time > skinnedMesh.totalAnimationTime)
				shotgunFiring = false;




			//player.position = camera.Position;
			player.Update(&camera, deltaTime);


			// Check collisions
			if (!NoClip)
			{
				player.HandleBoundingBoxCollisions(boundingBoxPtrs);
				player.HandleBoundingPlaneCollisions(boundingPlanePtrs);
			}

			RenderableObject::SetPositionByName("Sphere", player.position);




			//camera.Position = player.position;
			camera.CalculateviewPosition(player.GetViewPosition());
			camera.CalculateMatrices();


			// DOORS
			for (Door & door : house.doors)
				door.Update(deltaTime);

			Shader* lampShader = Shader::GetShaderByName("LampShader");
			Shader* debugFboShader = Shader::GetShaderByName("debugFboShader");
			Shader* geometryShader = Shader::GetShaderByName("Geometry");
			Shader* simpleDepthShader = Shader::GetShaderByName("SimpleDepth");
			Shader* shadowShader = Shader::GetShaderByName("Shadow");
			Shader* skyboxShader = Shader::GetShaderByName("Skybox");
			Shader* lightingShader = Shader::GetShaderByName("Lighting");
			Shader* FxaaShader = Shader::GetShaderByName("Fxaa");
			Shader* compositeShader = Shader::GetShaderByName("Composite");
			Shader* nullTechniqueShader = Shader::GetShaderByName("NullTechnique");
			Shader* finalShader = Shader::GetShaderByName("Final");
			Shader* downScaleShader = Shader::GetShaderByName("DownScale");

			Shader* blurHorizontalShader = Shader::GetShaderByName("BlurHorizontal");
			Shader* blurVerticalShader = Shader::GetShaderByName("BlurVertical");
			Shader* debugShader = Shader::GetShaderByName("DEBUG");
			Shader* DOFShader = Shader::GetShaderByName("DOF"); 
			Shader* decalShader = Shader::GetShaderByName("Decals");

		


			// Clear default buffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			/////////////////////////////////////////////////////////////////
			//						Prepare House						   //
			/////////////////////////////////////////////////////////////////

			bool preparehouse = true;
			if (preparehouse)
			{
				Light::lights.clear();

				for (Room& room : house.rooms)
				{
					room.CalculateLightLimits();
					Light::lights.push_back(&room.light);
				}
			}
			


		/////////////////////////////////////////////////////////////////
		//						Render passes						//
		/////////////////////////////////////////////////////////////////

		ShadowMapPass(simpleDepthShader);
		GeometryPass(geometryShader);
		DecalPass(decalShader);

		glBindFramebuffer(GL_FRAMEBUFFER, lightingBuffer.ID);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < Light::lights.size(); i++) {
			StencilPass(nullTechniqueShader, i);
			LightingPass(lightingShader, i);
		}

		CompositePass(compositeShader);
			
		BlurPass(blurVerticalShader, blurHorizontalShader);
		FinalPass(finalShader);
		DOFPass(DOFShader);


			//////////////////////////////////////////////////////
			//													//
			//					FORWARD	RENDERING				//
			//													//
			//////////////////////////////////////////////////////

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Deferred Debug
			if (showBuffers)
			{
				glUseProgram(debugFboShader->ID);
				glActiveTexture(GL_TEXTURE0);

				glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedoSpec);
				glViewport(0, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, gBuffer.gEmmisive);
				glViewport(SCR_WIDTH / 2, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);
				glViewport(0, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, lightingBuffer.gDOF);
				glViewport(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);


		/*		glBindTexture(GL_TEXTURE_2D, blurBuffers[0].textureA);
				glViewport(0, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[1].textureA);
				glViewport(SCR_WIDTH / 2, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[2].textureA);
				glViewport(0, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[3].textureA);
				glViewport(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
				Quad2D::RenderQuad(debugFboShader);*/


			/*	for (int x = 0; x < 4; x++)
				{
					glBindTexture(GL_TEXTURE_2D, blurBuffers[x].textureA);
					glViewport(SCR_WIDTH / 4 * x, 0, SCR_WIDTH / 4 - 1, SCR_HEIGHT / 4);
					Quad2D::RenderQuad(debugFboShader);

					glBindTexture(GL_TEXTURE_2D, blurBuffers[x].textureB);
					glViewport(SCR_WIDTH / 4 * x, SCR_HEIGHT / 4 + 1, SCR_WIDTH / 4 - 1, SCR_HEIGHT / 4);
					Quad2D::RenderQuad(debugFboShader);
				}*.

				/*glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[1].textureA);
				Quad2D::RenderQuad(debugFboShader);*/
			}



			// Draw final image
			else
			{
				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				debugFboShader->use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lightingBuffer.gLighting);
				//glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedoSpec);
				glBindTexture(GL_TEXTURE_2D, lightingBuffer.gDOF);
				//glBindTexture(GL_TEXTURE_2D, gBuffer.gEmmisive);
				//glBindTexture(GL_TEXTURE_2D, Texture::GetIDByName("bebop.png"));
				glDisable(GL_DEPTH_TEST); 
				Quad2D::RenderQuad(debugFboShader);
				glEnable(GL_DEPTH_TEST);


			/*	glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.ID);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST); 
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				*/

				/*
				graphiteToday at 07:56
					Honestly, having written four engines now, two personal, two professional.For shadow mapping what you want is two position - only buffers, one for static geometryand one for dynamic geometry.Things in the environment that are staticand will never move, like your worldand map models, those you bake directly down into the static buffer.Doing the vertex* matrix transform on the CPU, shoving it into that buffer.Dynamic stuff you just put all inside the dynamic position - only buffer as - is.When you do your shadow mapping pass, you calculate an index list of the geometry in the range of the light(only if the light moves, keep the same index list if the light did not move) and you render all your statics thenand there as one draw call.Now you enumerate the dynamic objects in the dynamic bufferand just render all of them(after frustum culling) those have to be rendered regardless even if the light does not move because they can end up inside any light volume.
					This is the only method that scalesand works.
					So the cost is basically one draw call for static geometry per light(or 6 for omni - directional point lights) + n draw calls for the dynamic objects
					Which is pretty much unbeatable without using multiple viewports
					So you can set an upper bound on your scene complexity this way too.Shadow draw calls will be(numSpotLights + numDirectionalLights + numPointLights * 6)* (nDynamicObjectsInView - 1)
					Which is really not that bad
					Oh yeah, those are lights in view too of course
					*/
				// Show lights
				lampShader->use();
				lampShader->setMat4("projection", camera.projectionMatrix);
				lampShader->setMat4("view", camera.viewMatrix);


			//	DrawPoint(lampShader, finalShellPos, glm::mat4(1), glm::vec3(0, 1, 1));
				//DrawPoint(lampShader, skinnedMesh.boltPos, glm::mat4(1), glm::vec3(0, 1, 1));

				// Draw lights
				if (showLights) {
					for (Light * light : Light::lights)
					{
						lightCube.transform.position = light->position;

						lampShader->setVec3("color", light->color);
						lightCube.Draw(lampShader, false);
					}
				}

				if (showRaycastPlane && raycastData.planeIndex != -1)
				{
					lampShader->setVec3("color", HELL_YELLOW);
					boundingPlanePtrs[raycastData.planeIndex]->Draw(lampShader);
				}

				// Draw bounding boxes
				if (showBoundingBoxes) {
					lampShader->setVec3("color", HELL_YELLOW);
					glDisable(GL_DEPTH_TEST);

					for (BoundingBox* & b : boundingBoxPtrs)
						b->Draw(lampShader);

					for (BoundingPlane* & p : boundingPlanePtrs)
						p->Draw(lampShader);
					
					// player sphere
					RenderableObject r = RenderableObject(Model::GetByName("sphere.obj"), "NO TEXTURE", player.position, glm::vec3(0), glm::vec3(0.1f));
					r.Draw(lampShader, false);

					glEnable(GL_DEPTH_TEST);
				}
				// Debug light volumes
				if (showVolumes) {

					glDisable(GL_DEPTH_TEST);
					glEnable(GL_CULL_FACE);

					//glDisable(GL_CULL_FACE);


					lampShader->setVec3("color", glm::vec3(1, 0, 0));

					// Render main room
					house.rooms[0].lightVolume.Draw(lampShader, GL_TRIANGLES);

					// Render door way volumes
					for (Room & room : house.rooms)
					{
						for (int i = 0; i < room.doors_BackWall.size(); i++)
							if (room.doors_BackWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
								room.backWallDoorwayLightVolumes[i].Draw(lampShader, GL_TRIANGLES);

						for (int i = 0; i < room.doors_FrontWall.size(); i++)
							if (room.doors_FrontWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
								room.frontWallDoorwayLightVolumes[i].Draw(lampShader, GL_TRIANGLES);

						for (int i = 0; i < room.doors_LeftWall.size(); i++)
							if (room.doors_LeftWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
								room.leftWallDoorwayLightVolumes[i].Draw(lampShader, GL_TRIANGLES);

						for (int i = 0; i < room.doors_RightWall.size(); i++)
							if (room.doors_RightWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
								room.rightWallDoorwayLightVolumes[i].Draw(lampShader, GL_TRIANGLES);
					}


					//glEnable(GL_CULL_FACE);
					glEnable(GL_DEPTH_TEST);
				}

				// CROSSHAIR
				CrosshairPass(debugFboShader, 14);
			}

			// SKYBOX DEBUG
			//DrawSkybox(skyboxShader);

			// Update Mouse Pick ID
			mousePicked = GetMousePicked();

			//std::cout << Texture::GetIDByName("door.png") << ": YEH WHAT\n";
		}


		void FxaaPass(Shader *shader)
		{

		}

		void StencilPass(Shader *shader, int lightIndex)
		{
			Light* light = Light::lights[lightIndex];
			glm::mat4 modelMatrix = glm::mat4(1);
			//modelMatrix = glm::translate(modelMatrix, light->position);
			//modelMatrix = glm::scale(modelMatrix, glm::vec3(light->radius));
			glm::mat4 gWVP = camera.projectionMatrix * camera.viewMatrix * modelMatrix;

			shader->use();
			shader->setMat4("gWVP", gWVP);
			
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.ID);
			glDrawBuffer(GL_NONE);

			glEnable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glClear(GL_STENCIL_BUFFER_BIT);
			glStencilFunc(GL_ALWAYS, 0, 0);
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

			
			// Render main room
			Room* room = &house.rooms[lightIndex];
			house.rooms[lightIndex].lightVolume.Draw(shader, GL_TRIANGLES);

			// Render door way volumes
			for (int i = 0; i < room->doors_BackWall.size(); i++)
				if (room->doors_BackWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
					room->backWallDoorwayLightVolumes[i].Draw(shader, GL_TRIANGLES);

			for (int i = 0; i < room->doors_FrontWall.size(); i++)
				if (room->doors_FrontWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
					room->frontWallDoorwayLightVolumes[i].Draw(shader, GL_TRIANGLES);

			for (int i = 0; i < room->doors_LeftWall.size(); i++)
				if (room->doors_LeftWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
					room->leftWallDoorwayLightVolumes[i].Draw(shader, GL_TRIANGLES);

			for (int i = 0; i < room->doors_RightWall.size(); i++)
				if (room->doors_RightWall[i]->doorStatus != DoorStatus::DOOR_CLOSED)
					room->rightWallDoorwayLightVolumes[i].Draw(shader, GL_TRIANGLES);
		}

		void LightingPass(Shader *shader, int lightIndex)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.ID);
			unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
			glDrawBuffers(5, attachments);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedoSpec);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gEmmisive);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowCubeMapArray.depthCubemap);

			shader->use();
			shader->setMat4("inverseProjectionMatrix", glm::inverse(camera.projectionMatrix));
			shader->setMat4("inverseViewMatrix", glm::inverse(camera.viewMatrix));
			shader->setVec3("viewPos", camera.Position);
			shader->setFloat("screenWidth", SCR_WIDTH);
			shader->setFloat("screenHeight", SCR_HEIGHT);
			shader->setInt("shadows", true);
			shader->setFloat("far_plane", ShadowCubeMapArray::SHADOW_FAR_PLANE);
			shader->setFloat("bias", bias);

			int i = lightIndex;
			Light* light = Light::lights[i]; 
			shader->setVec3("lightPositions[" + std::to_string(i) + "]", light->position);
			shader->setVec3("lightColors[" + std::to_string(i) + "]", light->color);
			shader->setFloat("lightStrengths[" + std::to_string(i) + "]", light->strength);
			shader->setFloat("lightRadius[" + std::to_string(i) + "]", light->radius);
			shader->setFloat("lightAttenuationExp[" + std::to_string(i) + "]", light->attExp);
			shader->setFloat("lightAttenuationLinear[" + std::to_string(i) + "]", light->attLinear);
			shader->setFloat("lightAttenuationConstant[" + std::to_string(i) + "]", light->attConstant);
			shader->setFloat("lowerXlimit", light->lowerXlimit);
			shader->setFloat("upperXlimit", light->upperXlimit);
			shader->setFloat("lowerZlimit", light->lowerZlimit);
			shader->setFloat("upperZlimit", light->upperZlimit);

			glStencilFunc(GL_NOTEQUAL, 0, 0xFF);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);

			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);


			// LIGHTING PASS
			glm::mat4 modelMatrix = glm::mat4(1);
			modelMatrix = glm::translate(modelMatrix, light->position);
			modelMatrix = glm::scale(modelMatrix, glm::vec3(light->radius));
			glm::mat4 gWVP = camera.projectionMatrix * camera.viewMatrix * modelMatrix;
			shader->setInt("lightIndex", i);
			shader->setMat4("gWVP", gWVP);
			RenderableObject sphere = RenderableObject(Model::GetByName("SphereLight.obj"), "red.png", light->position, glm::vec3(0), glm::vec3(light->radius));
			sphere.Draw(shader, false);

			glCullFace(GL_BACK);
			glDisable(GL_BLEND);

			glDisable(GL_STENCIL_TEST);
		}

		void CompositePass(Shader* shader)
		{
			glEnable(GL_CULL_FACE);	// undo damage
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);

			glDisable(GL_BLEND);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, lightingBuffer.ID);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			shader->use();
			shader->setFloat("brightness", brightness);
			shader->setFloat("contrast", contrast);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedoSpec);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gLighting);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gEmmisive);



			glDisable(GL_DEPTH_TEST); // MIGHT NOT BE NECCESAYR
			//glEnable(GL_BLEND);
			//glBlendEquation(GL_FUNC_ADD);
			//glBlendFunc(GL_ONE, GL_ONE);
;
				Quad2D::RenderQuad(shader);

			glDisable(GL_BLEND);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_DEPTH_TEST);

		}

		void BlurPass(Shader* blurVerticalShader, Shader* blurHorizontalShader)
		{
			// clear blur fuffer
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[0].ID);
			glClear(GL_DEPTH_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[1].ID);
			glClear(GL_DEPTH_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[2].ID);
			glClear(GL_DEPTH_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[3].ID);
			glClear(GL_DEPTH_BUFFER_BIT);
			//glDisable(GL_DEPTH_TEST);

			// Source
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.ID);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			int factor = 2;

			// Destination
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blurBuffers[0].ID);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

			// Blit
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor, GL_COLOR_BUFFER_BIT, GL_LINEAR);
				
			// Blur horizontal
			glViewport(0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor);
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[0].ID);	
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[0].textureA);
			blurHorizontalShader->use();
			blurHorizontalShader->setFloat("targetWidth", SCR_WIDTH / factor);
			Quad2D::RenderQuad(blurHorizontalShader);
	
			// Blur vertical
			glViewport(0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor);
			glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[0].ID);
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[0].textureB);
			blurVerticalShader->use();
			blurVerticalShader->setFloat("targetHeight", SCR_HEIGHT / factor);
			Quad2D::RenderQuad(blurVerticalShader);
			

			// second downscale //
			
			for (int i = 1; i < 4; i++)
			{
				factor *= 2;
				// Source
				glBindFramebuffer(GL_READ_FRAMEBUFFER, blurBuffers[i - 1].ID);
				glReadBuffer(GL_COLOR_ATTACHMENT0);

				// Destination
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blurBuffers[i].ID);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);

				// Blit
				glBlitFramebuffer(0, 0, SCR_WIDTH / (factor / 2), SCR_HEIGHT / (factor / 2), 0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor, GL_COLOR_BUFFER_BIT, GL_LINEAR);

				// Blur horizontal
				glViewport(0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor);
				glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[i].ID);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glDrawBuffer(GL_COLOR_ATTACHMENT1);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[i].textureA);
				blurHorizontalShader->use();
				blurHorizontalShader->setFloat("targetWidth", SCR_WIDTH / factor);
				Quad2D::RenderQuad(blurHorizontalShader);

				// Blur vertical
				glViewport(0, 0, SCR_WIDTH / factor, SCR_HEIGHT / factor);
				glBindFramebuffer(GL_FRAMEBUFFER, blurBuffers[i].ID);
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, blurBuffers[i].textureB);
				blurVerticalShader->use();
				blurVerticalShader->setFloat("targetHeight", SCR_HEIGHT / factor);
				Quad2D::RenderQuad(blurVerticalShader);

			}
		}

		bool bloom = true;
		float exposure = 1.0f;

		void FinalPass(Shader* shader)
		{
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			shader->use();
			glBindFramebuffer(GL_FRAMEBUFFER, lightingBuffer.ID);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, lightingBuffer.gComposite);
			// glDrawBuffer(GL_COLOR_ATTACHMENT0); // draw to colour acttachment 1

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[0].textureA);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[1].textureA);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[2].textureA);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, blurBuffers[3].textureA);
			
			shader->setInt("bloom", bloom);
			shader->setFloat("exposure", exposure); 
			Quad2D::RenderQuad(shader);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void DOFPass(Shader* shader)
		{
			//return;

			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			shader->use();
			glBindFramebuffer(GL_FRAMEBUFFER, lightingBuffer.ID);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, lightingBuffer.gLighting);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);	

			unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
			glDrawBuffers(3, attachments);


			shader->setFloat("screenWidth", SCR_WIDTH);
			shader->setFloat("screenHeight", SCR_HEIGHT);

			shader->setBool("showFocus", Config::DOF_showFocus);
			shader->setBool("vignetting", Config::DOF_vignetting);
			shader->setFloat("vignout", Config::DOF_vignout);
			shader->setFloat("vignin", Config::DOF_vignin);
			shader->setFloat("vignfade", Config::DOF_vignfade);
			shader->setFloat("CoC", Config::DOF_CoC);
			shader->setFloat("maxblur", Config::DOF_maxblur);
			shader->setInt("samples", Config::DOF_samples);
			shader->setInt("samples", Config::DOF_samples);
			shader->setInt("rings", Config::DOF_rings);
			shader->setFloat("threshold", Config::DOF_threshold);
			shader->setFloat("gain", Config::DOF_gain);
			shader->setFloat("bias", Config::DOF_bias);
			shader->setFloat("fringe", Config::DOF_fringe);

			Quad2D::RenderQuad(shader);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			//glEnable (GL_DEPTH_TEST);
			//glEnable(GL_BLEND);
		}

		void GeometryPass(Shader *shader)
		{
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.ID);
			unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
			glDrawBuffers(5, attachments);
			
			glDepthMask(GL_TRUE); 
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			shader->use();
			shader->setMat4("pv", camera.projectionViewMatrix);
			shader->setFloat("TEXTURE_SCALE", 1.0);		// idk the best way to handles this. but for now its here.
			DrawScene(shader, true);

			// Commented out below cause now you're drawing decals into the GBuffer right after this
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glDepthMask(GL_FALSE);
		}

		

		void DecalPass(Shader* shader)
		{

			/*Cube cube = Cube(raycastData.intersectionPoint, glm::vec3(1, 1, 1) * 0.025f);
			glm::vec3 normal = glm::vec3(0, 0, 1);
			if (raycastData.planeIndex != -1)
				normal = boundingPlanePtrs[raycastData.planeIndex]->normal;*/
			
			shader->use();
			shader->setMat4("pv", camera.projectionViewMatrix);
			shader->setMat4("inverseProjectionMatrix", glm::inverse(camera.projectionMatrix));
			shader->setMat4("inverseViewMatrix", glm::inverse(camera.viewMatrix));
			shader->setFloat("screenWidth", SCR_WIDTH);
			shader->setFloat("screenHeight", SCR_HEIGHT);
			
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gBuffer.rboDepth);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);

			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glEnable (GL_DEPTH_TEST);
			//glDisable(GL_DEPTH_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			shader->setInt("writeRoughnessMetallic", 0);
			for (Decal decal : decals) {
				shader->setVec3("targetPlaneSurfaceNormal", decal.normal);
				decal.Draw(shader, false);
			}
			shader->setInt("writeRoughnessMetallic", 1);
			for (Decal decal : decals) {
				shader->setVec3("targetPlaneSurfaceNormal", decal.normal);
				decal.Draw(shader, true);
			}

			//if (showRaycastPlane)
			//	cube.DecalDraw(shader, normal);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthMask(GL_FALSE);
		}


		void ShadowMapPass(Shader *shader)
		{
			//Timer timer("Shadowmap pass");
			glViewport(0, 0, ShadowCubeMapArray::SHADOW_SIZE, ShadowCubeMapArray::SHADOW_SIZE);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowCubeMapArray.ID);
			glClear(GL_DEPTH_BUFFER_BIT);


			shader->use();
			shader->setFloat("far_plane", ShadowCubeMapArray::SHADOW_FAR_PLANE);

			for (int j = 0; j < Light::lights.size(); j++)
			{
				shader->setVec3("lightPositions[" + std::to_string(j) + "]", Light::lights[j]->position);
				shader->setInt("lightIndex", j);
				for (unsigned int i = 0; i < 6; ++i)
					shader->setMat4("shadowMatrices[" + std::to_string(i) + "]", Light::lights[j]->shadowTransforms[i]);
				//glEnable(GL_CULL_FACE);
				//glCullFace(GL_FRONT);
				DrawScene(shader, false);
				//glCullFace(GL_BACK);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void CrosshairPass(Shader *shader, int crosshairSize)
		{
			int texID = -1;
			if (mousePicked.type == MousePickType::Door)
				texID = Texture::GetIDByName("crosshair_interact.png");
			else
				texID = Texture::GetIDByName("crosshair_cross.png");

			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			shader->use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texID);
			//glViewport(SCR_WIDTH / 2 - (crosshairSize / 2), SCR_HEIGHT / 2 - (crosshairSize / 2), crosshairSize, crosshairSize);
			Quad2D::RenderCrosshair(shader, SCR_WIDTH, SCR_HEIGHT, crosshairSize);

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
		}

		void DrawSkybox(Shader *shader)
		{
			return;
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDepthFunc(GL_LEQUAL);
			shader->use();
			shader->setMat4("view", camera.viewMatrix);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowCubeMapArray.depthCubemap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowCubeMapArray.depthCubemap);
			//	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowBuffer.depthCubemap);
			skybox.Draw(shader, true);
			glDepthFunc(GL_LESS);
		}

		void DrawScene(Shader *shader, bool bindTextures)
		{
			house.DrawAll(shader, bindTextures);
			RenderableObject::DrawAll(shader, bindTextures);

			RenderableObject r;
			r.model == Model::GetByName("Shell.fbx");


			for (Shell shell : shells)
			 shell.Draw(shader, bindTextures);
			

			// SHOTGUN HUD
			if (bindTextures)
			{
				Util::OUTPUT_TEXT = "";
				Util::OUTPUT("Time: " + std::to_string(time));

				vector<glm::mat4> Transforms;

				if (time < skinnedMesh.totalAnimationTime)
					skinnedMesh.BoneTransform(time, Transforms);
				else
					skinnedMesh.BoneTransform(0, Transforms);

				for (unsigned int i = 0; i < Transforms.size(); i++)
					shader->setMat4("jointTransforms[" + std::to_string(i) + "]", glm::transpose(Transforms[i]));

				skinnedMesh.Render(shader, camera);

				shader->setInt("animated", false);
				//////////////////////////////////////////////////////////////////////////////////
			}
		}

		
		virtual void OnImGuiRender() override
		{
			if (!showImGUI)
				return;

			HellEngine::Application &app = HellEngine::Application::Get();

			ImGuiIO* io = &(ImGui::GetIO());
			int fps = std::round(io->Framerate);
			std::string fpsText = "FPS: " + std::to_string(fps);

			//const char * Types[] = { "NotFound", "Wall", "Door"}; 

			int debugWindowWidth = 480;
			int comboWidth = 177;
			int comboWidth2 = 140;
			int lightOptionsWidth = 205;
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

			ImGui::SetNextWindowSize(ImVec2(debugWindowWidth, SCR_HEIGHT - 40));
			//ImGui::SetNextWindowPos(ImVec2(32, 32));
			ImGui::Begin("Test", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			ImGui::Text("WELCOME TO HELL");
			ImGui::Text(fpsText.c_str());

			// Player pos
			std::string p = "Player Pos: " + std::to_string(player.position.x) + ", " + std::to_string(player.position.y) + ", " + std::to_string(player.position.z);
			ImGui::Text(p.c_str());
			ImGui::Text(" ");

			if (raycastData.planeIndex != -1)
			Util::OUTPUT("raycast plane: " + Util::Vec3ToString(boundingPlanePtrs[raycastData.planeIndex]->normal));

			ImGui::Text(Util::OUTPUT_TEXT.c_str());


			//////////////////
			// Main menu yo //
			//////////////////

		//	ImguiFloat3("Cam Pos:   ", &camera.Position);
		//	ImguiFloat3("Cam Front: ", &camera.Front);
		//	ImguiFloat3("Ray thing: ", &ray_wor);

			//ImGui::Text(("DistanceAtoB: " + std::to_string(a)).c_str());
			//ImGui::Text(("intersectionX: " + std::to_string(b)).c_str());


			/*ImGui::Text("sway amount"); ImGui::SameLine();
			ImGui::InputFloat("##G", &skinnedMesh.swayAmount, 0.0f, 9.0f, 10.0f);
			ImGui::Text("smooth amount"); ImGui::SameLine();
			ImGui::InputFloat("##Gf", &skinnedMesh.smoothAmount, 0.0f, 9.0f, 10.0f);
			ImGui::Text("min X"); ImGui::SameLine();
			ImGui::InputFloat("##fGf", &skinnedMesh.min_X, 0.0f, 9.0f, 10.0f);
			ImGui::Text("man X"); ImGui::SameLine();
			ImGui::InputFloat("##ffGf", &skinnedMesh.max_X, 0.0f, 9.0f, 10.0f);
			ImGui::Text("min Y"); ImGui::SameLine();
			ImGui::InputFloat("##fgGf", &skinnedMesh.min_Y, 0.0f, 9.0f, 10.0f);
			ImGui::Text("man Y"); ImGui::SameLine();
			ImGui::InputFloat("##fqwefGf", &skinnedMesh.max_Y, 0.0f, 9.0f, 10.0f);*/

			/*
			ImGui::Text("translation"); ImGui::SameLine();
			ImGui::InputFloat3("##B", glm::value_ptr(skinnedMesh.meshTranslation), -10, 10);

			ImGui::Text("rotation angle"); ImGui::SameLine();
			ImGui::InputFloat3("##A", glm::value_ptr(skinnedMesh.meshRotation), -10, 10);

			ImGui::Text("scale"); ImGui::SameLine();
			ImGui::InputFloat("##G", &skinnedMesh.meshScale, 0.0f, 9.0f, 10.0f);

			ImGui::Text("headbob factor X"); ImGui::SameLine();
			ImGui::InputFloat("#qwe#ghh", &skinnedMesh.headBobFactorX, 0.0f, 9.0f, 10.0f);
			ImGui::Text("headbob factor Y"); ImGui::SameLine();
			ImGui::InputFloat("##gfdshh", &skinnedMesh.headBobFactorY, 0.0f, 9.0f, 10.0f);
			ImGui::Text("headbob speed"); ImGui::SameLine();
			ImGui::InputFloat("##gfhh", &skinnedMesh.headBobSpeed, 0.0f, 9.0f, 10.0f);


			ImGui::Text("rotation amount"); ImGui::SameLine();
			ImGui::SliderFloat("##C", &skinnedMesh.meshRotAngle, 0.0f, 5.0f);
			
			
			ImGui::Text("SHELL FORWARD"); ImGui::SameLine();
			ImGui::InputFloat("##fdG", &Shell::shellForwardFactor, 0.0f, 9.0f, 10.0f);

			ImGui::Text("SHELL up"); ImGui::SameLine();
			ImGui::InputFloat("##fdqweG", &Shell::shellUpFactor, 0.0f, 9.0f, 10.0f);

			ImGui::Text("SHELL right"); ImGui::SameLine();
			ImGui::InputFloat("##fsdfdG", &Shell::shellRightFactor, 0.0f, 9.0f, 10.0f);

			ImGui::Text("SHELL SPEED RIGHT"); ImGui::SameLine();
			ImGui::InputFloat("##fdqqweqweweG", &Shell::shellSpeedRight, 0.0f, 9.0f, 10.0f);

			ImGui::Text("SHELL SPEED UP"); ImGui::SameLine();
			ImGui::InputFloat("##fsdffaffdG", &Shell::shellSpeedUp, 0.0f, 9.0f, 10.0f);

			ImGui::Text("SHELL GRAV"); ImGui::SameLine();
			ImGui::InputFloat("##fsdffawerffdG", &Shell::shellGravity, 0.0f, 9.0f, 10.0f);
			

			std::string s;
			s = "BOLT: " + std::to_string(skinnedMesh.boltPos.x);
			s += ", " + std::to_string(skinnedMesh.boltPos.y);
			s += ", " + std::to_string(skinnedMesh.boltPos.z);
			ImGui::Text(s.c_str());


			s = "FRONT: " + std::to_string(camera.Front.x);
			s += ", " + std::to_string(camera.Front.y);
			s += ", " + std::to_string(camera.Front.z);
			ImGui::Text(s.c_str());

			s = "UP: " + std::to_string(camera.Up.x);
			s += ", " + std::to_string(camera.Up.y);
			s += ", " + std::to_string(camera.Up.z);
			ImGui::Text(s.c_str());

			*/

			

		//	ImGui::End();
		//	return;
			
			ImGui::BeginTabBar("MASTER_TAB_BAR", tab_bar_flags);
			ImGui::Text("\n");

			if (ImGui::BeginTabItem("Info")) {
				InfoMenu();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Player")) {
				PlayerMenu();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Game")) {
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Shaders")) {
				ShaderMenu();
				ImGui::EndTabItem();
			}							
			if (ImGui::BeginTabItem("Map")) 	{
				MapMenu();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();				
			ImGui::End();

			//ImGui::SliderFloat("lowerXlimit", &lowerXlimit, 0.0f, 20.0f);
			//ImGui::SliderFloat("upperXlimit", &upperXlimit, 0.0f, 20.0f);
			//ImGui::SliderFloat("lowerZlimit", &lowerZlimit, 0.0f, 20.0f);
			//ImGui::SliderFloat("upperZlimit", &upperZlimit, 0.0f, 20.0f);
			/*
			ImGui::SliderFloat("brightness", &brightness, -0.5f, 0.5f);
			ImGui::SliderFloat("contrast", &contrast, -0.0f, 2.0f);
			ImGui::SliderFloat("bias", &bias, -0.2f, 1.0f);*/
		}

		void InfoMenu()
		{
			ImGui::Text("KEYS");
			ImGui::Text("WASD: movement");
			ImGui::Text("Space: jump");
			ImGui::Text("Ctrl: crouch");
			ImGui::Text("F: toggle fullscreen");
			ImGui::Text("B: toggle bounding boxes");
			ImGui::Text("R: toggle raycast plane");
			ImGui::Text("M: toggle mouse usage");
			ImGui::Text("C: toggle clipping");
			ImGui::Text("C: also clears decals");
			ImGui::Text("I: hide/show this box");
		}

		void PlayerMenu()
		{
			ImGui::PushItemWidth(50);
			ImGui::InputFloat("headbobSpeed", &camera.headBobSpeed);
			ImGui::InputFloat("headbobFactor", &camera.headBobFactor);
			ImGui::Text("\n");

			ImGui::InputFloat("crouchingSpeed", &player.crouchDownSpeed);
			ImGui::InputFloat("crouchingHeight", &player.couchingViewHeight);
			ImGui::InputFloat("standingHeight", &player.standingViewHeight);
			ImGui::Text(("Crouching: " + std::to_string(player.crouching)).c_str());
			ImGui::Text(("CurrentHeight: " + std::to_string(player.currentHeight)).c_str());
			ImGui::Text("\n");

			ImGui::InputFloat("Run Speed", &player.runningSpeed);
			ImGui::InputFloat("Walk Speed", &player.walkingSpeed);
			ImGui::InputFloat("Crouch Speed", &player.crouchingSpeed);
			ImGui::InputFloat("Jump Strength", &player.jumpStrength);
			ImGui::InputFloat("Gravity", &player.gravity);
			ImGui::Text(("Grounded: " + std::to_string(player.IsGrounded())).c_str());
			ImGui::Text("\n");

			ImGui::InputFloat("Approach Speed", &player.velocityApproachSpeed);
			ImGui::Text(std::string("Current Vel: " + Util::Vec3ToString(player.currentVelocity)).c_str());
			ImGui::Text(std::string("Target Vel:  " + Util::Vec3ToString(player.targetVelocity)).c_str());
			ImGui::Text("\n");

			ImGui::InputFloat("TARGET PITCH", &camera.TargetPitch);
			ImGui::InputFloat("TARGET YAW", &camera.TargetYaw);
			ImGui::InputFloat("CURRENT PITCH", &camera.CurrentPitch);
			ImGui::InputFloat("CURRENT YAW", &camera.CurrentYaw);
			ImGui::InputFloat("ROTATION SPEED", &camera.rotationSpeed);


			if (ImGui::InputFloat("Light_Volume_Bias", &Room::Light_Volume_Bias))
				RebuildMap();


			ImGui::PushItemWidth(250);
			if (ImGui::InputFloat3("##B", glm::value_ptr(WorkBenchPosition), 2, 2))
				RenderableObject::SetPositionByName("Bench", WorkBenchPosition);
		}


		void ShaderMenu()
		{
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None; 
			ImGui::BeginTabBar("SHADER_TAB_BAR", tab_bar_flags);
			ImGui::Text("\n");

			if (ImGui::BeginTabItem("DOF")) {

				ImGui::SameLine(); ImGui::Checkbox("Show Focus##showFocus", &Config::DOF_showFocus);
				ImGui::SameLine(); ImGui::Checkbox("Vignetting##vignetting", &Config::DOF_vignetting);
				ImGui::SliderFloat("Vignette Out", &Config::DOF_vignout, 0.0f, 5.0f);
				ImGui::SliderFloat("Vignette In", &Config::DOF_vignin, 0.0f, 5.0f);
				ImGui::SliderFloat("Vignette Fade", &Config::DOF_vignfade, 0.0f, 200.0f);
				ImGui::SliderFloat("CoC", &Config::DOF_CoC, 0.0f, 5.0f);
				ImGui::SliderFloat("Max Blur", &Config::DOF_maxblur, 0.0f, 5.0f);
				ImGui::SliderInt("Samples", &Config::DOF_samples, 0, 10);
				ImGui::SliderInt("Rings", &Config::DOF_rings, 0, 10);
				ImGui::SliderFloat("Threshold", &Config::DOF_threshold, 0.0f, 5.0f);
				ImGui::SliderFloat("Gain", &Config::DOF_gain, 0.0f, 5.0f);
				ImGui::SliderFloat("Bias", &Config::DOF_bias, 0.0f, 5.0f);
				ImGui::SliderFloat("Fringe", &Config::DOF_fringe, 0.0f, 5.0f);


				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();

		}


		void MapMenu()
		{
			int comboWidth = 177;
			int comboWidth2 = 140;
			int lightOptionsWidth = 205;
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

			static vector<std::string> current_floorMaterial;
			static vector<std::string> current_DoorFloorMaterial;
			static vector<std::string> current_ceilingMaterial;
			static vector<std::string> current_wallMaterial;
			static vector<std::string> current_doorAxis;

			// Setup
			if (_IMGUI_RUN_ONCE)
			{
				current_floorMaterial.clear();
				current_ceilingMaterial.clear();
				current_wallMaterial.clear();
				current_DoorFloorMaterial.clear();
				current_doorAxis.clear();

				for (int i = 0; i < house.rooms.size(); i++)
				{
					current_floorMaterial.push_back(house.rooms[i].floor.material->name);
					current_ceilingMaterial.push_back(house.rooms[i].ceiling.material->name);
					current_wallMaterial.push_back(house.rooms[i].walls[0].material->name);
				}

				for (int i = 0; i < house.doors.size(); i++)
				{
					current_DoorFloorMaterial.push_back(house.doors[i].floor.material->name);
					current_doorAxis.push_back(Util::AxisToString(house.doors[i].axis));
				}
				_IMGUI_RUN_ONCE = false;
			}

			// Map list
			static std::string currentMap = "Level1.map";

			// Materials list
			std::vector<std::string> materialList;
			for (Material & material : Material::materials)
				materialList.push_back(material.name);

			// Axis list
			std::vector<std::string> axisList;
			axisList.push_back("X");
			axisList.push_back("X_NEGATIVE");
			axisList.push_back("Z");
			axisList.push_back("Z_NEGATIVE");
			
			// NEW LOAD SAVE MENU

			if (ImGui::BeginCombo("##MapList", currentMap.c_str()))
			{
				for (int n = 0; n < File::MapList.size(); n++)
				{
					bool is_selected = (currentMap == File::MapList[n]);
					if (ImGui::Selectable(File::MapList[n].c_str(), is_selected)) {
						//	house.rooms[i].SetWallMaterial(Material::GetMaterialByName(materialList[n]));
						currentMap = File::MapList[n];
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}


			if (ImGui::Button("New Map"))
				house.RemoveAllContents();

			ImGui::SameLine(); if (ImGui::Button("Load Map")) {
				house = File::LoadMap(currentMap);
				house.RebuildRooms();
				RebuildMap();

				camera.Position = glm::vec3(0.6f, 0.0f, 9.45f);
				player.position = glm::vec3(0.6f, 0.0f, 9.45f);
				camera.TargetYaw = 270.0f;
				camera.TargetPitch = -6.0f;
				HellEngine::Application::Get().GetWindow().ToggleMouseEnabled();
			}
			ImGui::SameLine(); if (ImGui::Button("Save Map"))
				File::SaveMap(currentMap, &house);

			ImGui::Text("\n");
			ImGui::Separator();
			ImGui::Text("\n");

			// NEw ROOMS AND DOOR

			ImGui::PushItemWidth(75);

			if (ImGui::Button("New Door")) {
				house.AddDoor(house.newDoorPosition.x, house.newDoorPosition.y, Axis::X, "FloorBoards", false);
				RebuildMap();
			}
			ImGui::SameLine(); ImGui::Text("At");
			ImGui::SameLine(); ImGui::InputFloat2("##NEWDOOR", glm::value_ptr(house.newDoorPosition), 1, 0);

			if (ImGui::Button("New Room"))
			{
				house.AddRoom(
					glm::vec3(house.newRoomCornerAPosition.x, 0, house.newRoomCornerAPosition.y),
					glm::vec3(house.newRoomCornerBPosition.x, 0, house.newRoomCornerBPosition.y),
					"Wall",
					"FloorBoards",
					"Concrete2",
					false,
					false);
				RebuildMap();
			}

			ImGui::SameLine(); ImGui::Text("Inner corner");
			ImGui::SameLine(); ImGui::InputFloat2("##CORNERA", glm::value_ptr(house.newRoomCornerAPosition), 1, 0);
			ImGui::SameLine(); ImGui::Text("Outer corner");
			ImGui::SameLine(); ImGui::InputFloat2("##CORNERB", glm::value_ptr(house.newRoomCornerBPosition), 1, 0);


			// ROOOOOOOOOMS TAB
			if (house.rooms.size() > 0)
			{
				ImGui::Text("\n");

				ImGui::BeginTabBar("MyTabBar", tab_bar_flags);
				for (int i = 0; i < house.rooms.size(); i++)
				{
					// ROOMS
					if (ImGui::BeginTabItem(("Room " + std::to_string(i)).c_str()))
					{
						// MATERIALS
						{
							ImGui::Text("\n");
							ImGui::PushItemWidth(comboWidth);

							// Size
							ImGui::Text("North Wall");
							ImGui::SameLine(); if (ImGui::Button("<##A")) {
								house.rooms[i].cornerA.z -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine(); if (ImGui::Button(">##B")) {
								house.rooms[i].cornerA.z += 0.1;
								RebuildMap();
							}
							ImGui::SameLine();

							ImGui::Text("East Wall");
							ImGui::SameLine(); if (ImGui::Button("<##C")) {
								house.rooms[i].cornerA.x -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine(); if (ImGui::Button(">##D")) {
								house.rooms[i].cornerA.x += 0.1;
								RebuildMap();
							}
							ImGui::Text("South Wall");
							ImGui::SameLine(); if (ImGui::Button("<##E")) {
								house.rooms[i].cornerB.z -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine(); if (ImGui::Button(">##F")) {
								house.rooms[i].cornerB.z += 0.1;
								RebuildMap();
							}
							ImGui::SameLine();

							ImGui::Text("West Wall");
							ImGui::SameLine(); if (ImGui::Button("<##G")) {
								house.rooms[i].cornerB.x -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine(); if (ImGui::Button(">##H")) {
								house.rooms[i].cornerB.x += 0.1;
								RebuildMap();
							}

							// Walls
							ImGui::Text("Walls  ");
							ImGui::SameLine();
							if (ImGui::BeginCombo("##combo3", current_wallMaterial[i].c_str()))
							{
								for (int n = 0; n < materialList.size(); n++)
								{
									bool is_selected = (current_wallMaterial[i] == materialList[n]);
									if (ImGui::Selectable(materialList[n].c_str(), is_selected)) {
										house.rooms[i].SetWallMaterial(Material::GetMaterialByName(materialList[n]));
										current_wallMaterial[i] = materialList[n];
									}
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}

							// Floor
							ImGui::Text("Floor  ");
							ImGui::SameLine();
							if (ImGui::BeginCombo("##combo", current_floorMaterial[i].c_str()))
							{
								for (int n = 0; n < materialList.size(); n++)
								{
									bool is_selected = (current_floorMaterial[i] == materialList[n]);
									if (ImGui::Selectable(materialList[n].c_str(), is_selected)) {
										house.rooms[i].floor.material = Material::GetMaterialByName(materialList[n]);
										current_floorMaterial[i] = materialList[n];
									}
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							ImGui::SameLine(); ImGui::Checkbox("##checkBox", &house.rooms[i].floor.rotateTexture);
							ImGui::SameLine(); ImGui::Text("Rotate");

							ImGui::Text("Floor Tex Scale"); ImGui::SameLine();
							ImGui::InputFloat("##floorscale", &house.rooms[i].floor.textureScale, 2, 0);
							
							// Ceiling
							ImGui::Text("Ceiling");
							ImGui::SameLine();
							if (ImGui::BeginCombo("##combo2", current_ceilingMaterial[i].c_str()))
							{
								for (int n = 0; n < materialList.size(); n++)
								{
									bool is_selected = (current_ceilingMaterial[i] == materialList[n]);
									if (ImGui::Selectable(materialList[n].c_str(), is_selected)) {
										house.rooms[i].ceiling.material = Material::GetMaterialByName(materialList[n]);
										current_ceilingMaterial[i] = materialList[n];
									}
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							ImGui::SameLine(); ImGui::Checkbox("##checkBox2", &house.rooms[i].ceiling.rotateTexture);
							ImGui::SameLine(); ImGui::Text("Rotate");

							ImGui::Text("Wall TexCoord Offset");
							ImGui::SameLine(); ImGui::InputFloat2("##TEX_X", glm::value_ptr(house.rooms[i].texOffset), 10, 0);
						}

						// LIGHTS ///this needs checkign
						{
							ImGui::Text("\n");
							ImGui::PushItemWidth(lightOptionsWidth);

							ImGui::Text("Light Color   "); ImGui::SameLine();
							if (ImGui::ColorEdit3("##A", glm::value_ptr(Light::lights[i]->color), i))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text("Light Position"); ImGui::SameLine();
							if (ImGui::InputFloat3("##B", glm::value_ptr(Light::lights[i]->position), -10, 10))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text("Light Constant"); ImGui::SameLine();
							if (ImGui::SliderFloat("##C", &Light::lights[i]->attConstant, 0.0f, 5.0f))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text("Light Linear  "); ImGui::SameLine();
							if (ImGui::SliderFloat("##D", &Light::lights[i]->attLinear, 0.0f, 5.0f))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text("Light Exp     "); ImGui::SameLine();
							if (ImGui::SliderFloat("##E", &Light::lights[i]->attExp, 0.0f, 5.0f))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text("Light Strength"); ImGui::SameLine();
							if (ImGui::SliderFloat("##F", &Light::lights[i]->strength, 0.0f, 50.0f))
								house.rooms[i].CreateLightVolumes();

							ImGui::Text(("Light Radius: " + std::to_string(Light::lights[i]->radius)).c_str());
						}

						ImGui::SameLine(); if (ImGui::Button("Re-center light##LIGHT")) {
							house.rooms[i].CenterLight();
						}

						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();

			}


			if (house.doors.size() > 0)
			{
				ImGui::Text("\n");

				ImGui::BeginTabBar("MyTabBar2", tab_bar_flags);
				for (int i = 0; i < house.doors.size(); i++)
				{
					ImGui::Text("\n");

					// DOORS
					if (ImGui::BeginTabItem(("Door " + std::to_string(i)).c_str()))
					{
						// MATERIALS
						{
							std::string x = Util::FloatToString(house.doors[i].position.x, 1);
							std::string z = Util::FloatToString(house.doors[i].position.z, 1);
							ImGui::Text(("Position: " + x + ", " + z).c_str());

							ImGui::PushItemWidth(comboWidth2);

							// Door axis
							ImGui::Text("Rotation Axis ");
							ImGui::SameLine();
							if (ImGui::BeginCombo("##combo7", current_doorAxis[i].c_str()))
							{
								for (int n = 0; n < axisList.size(); n++)
								{
									bool is_selected = (current_doorAxis[i] == axisList[n]);
									if (ImGui::Selectable(axisList[n].c_str(), is_selected)) {
										house.doors[i].axis = Util::StringToAxis(axisList[n]);
										current_doorAxis[i] = axisList[n];
									}
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}


							ImGui::Text("Move X");
							ImGui::SameLine();

							if (ImGui::Button("<##H")) {
								house.doors[i].position.x -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine();

							if (ImGui::Button(">##I")) {
								house.doors[i].position.x += 0.1;
								RebuildMap();
							}
							ImGui::Text("Move Z");
							ImGui::SameLine();

							if (ImGui::Button("<##J")) {
								house.doors[i].position.z -= 0.1;
								RebuildMap();
							}
							ImGui::SameLine();
							if (ImGui::Button(">##K")) {
								house.doors[i].position.z += 0.1;
								RebuildMap();
							}


							// Door floor material
							ImGui::Text("Floor Material");
							ImGui::SameLine();
						
		void ReportError(char* msg);	if (ImGui::BeginCombo("##combo5", current_DoorFloorMaterial[i].c_str()))
							{
								for (int n = 0; n < materialList.size(); n++)
								{
									bool is_selected = (current_DoorFloorMaterial[i] == materialList[n]);
									if (ImGui::Selectable(materialList[n].c_str(), is_selected)) {
										house.doors[i].floor.material = Material::GetMaterialByName(materialList[n]);
										current_DoorFloorMaterial[i] = materialList[n];
									}
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}

							ImGui::Text("Floor Rotate  ");
							ImGui::SameLine(); ImGui::Checkbox("##checkBox3", &house.doors[i].floor.rotateTexture);

						}
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}
		}


		void OnEvent(HellEngine::Event& event) override
		{
			if (event.GetEventType() == HellEngine::EventType::MouseButtonPressed)
			{
				if (!shotgunFiring)
				{
					time = 0;
					Audio::PlayAudio("Shotgun.wav");
					shellEjected = false;
					shotgunFiring = true;

					for (int i = 0; i < 12; i++) {
						raycastData = GetRaycastData(0.125f);
						if (raycastData.planeIndex != -1)
						decals.push_back(Decal(DecalType::BULLET_HOLE, raycastData.intersectionPoint, boundingPlanePtrs[raycastData.planeIndex]->normal));
					}
				}
			}

			// Key pressed
			if (event.GetEventType() == HellEngine::EventType::KeyPressed)
			{
				HellEngine::KeyPressedEvent& e = (HellEngine::KeyPressedEvent&)event;


				/*if (e.GetKeyCode() == HELL_KEY_1)
					Audio::LoadAudio("Music.mp3");
				if (e.GetKeyCode() == HELL_KEY_2)
					Audio::LoadAudio("Music2.mp3");
				if (e.GetKeyCode() == HELL_KEY_3)
					Audio::LoadAudio("Music3.mp3");*/

					// RESET TIME
				if (e.GetKeyCode() == HELL_KEY_T)
					time = 0;

				// RESET TIME
				if (e.GetKeyCode() == HELL_KEY_C)
				{
					decals.clear();
				}
					
				// toggle fullscreen
				if (e.GetKeyCode() == HELL_KEY_F)
				{
					HellEngine::Application::Get().GetWindow().ToggleFullscreen();
					ReCreateFBOs();
					camera.CalculateProjectionMatrix(SCR_WIDTH, SCR_HEIGHT);
				}
				// No Clip
				if (e.GetKeyCode() == HELL_KEY_C)
					NoClip = !NoClip;
				// show bounding boxes
				if (e.GetKeyCode() == HELL_KEY_B)
					showBoundingBoxes = !showBoundingBoxes;
				// Show buffers
				if (e.GetKeyCode() == HELL_KEY_CAPS_LOCK)
					showBuffers = true;
				// show lights
				if (e.GetKeyCode() == HELL_KEY_L)
					showLights = !showLights;
				// show ImGui
				if (e.GetKeyCode() == HELL_KEY_I)
					showImGUI = !showImGUI;
				// Optimize
				if (e.GetKeyCode() == HELL_KEY_O)
					optimise = !optimise;
				// Toggle mouse control
				if (e.GetKeyCode() == HELL_KEY_M)
					HellEngine::Application::Get().GetWindow().ToggleMouseEnabled();
				// Recalculate shit
				if (e.GetKeyCode() == HELL_KEY_R) {
					for (Light * light : Light::lights)
						light->CalculateShadowProjectionMatricies();


					for (Room & room : house.rooms)
						room.CreateLightVolumes();
				}
				// Light Volumes
				if (e.GetKeyCode() == HELL_KEY_V)
					showVolumes = !showVolumes;	
				
				// Raycast Plane
				if (e.GetKeyCode() == HELL_KEY_R)
					showRaycastPlane = !showRaycastPlane;

				// Weapons
				if (e.GetKeyCode() == HELL_KEY_1)
					selectedWeapon = 1;
				if (e.GetKeyCode() == HELL_KEY_2)
					selectedWeapon = 2;
				if (e.GetKeyCode() == HELL_KEY_3)
					selectedWeapon = 3;
				if (e.GetKeyCode() == HELL_KEY_4)
					selectedWeapon = 4;
				if (e.GetKeyCode() == HELL_KEY_5)
					selectedWeapon = 5;

				if (e.GetKeyCode() == HELL_KEY_E)
				{
					Interact();
				}

				if (e.GetKeyCode() == HELL_KEY_Q)
					for (Door& door : house.doors)
						door.Interact();
					
			}
			//Key released
			if (event.GetEventType() == HellEngine::EventType::KeyReleased)
			{
				HellEngine::KeyReleasedEvent& e = (HellEngine::KeyReleasedEvent&)event;

				// stop showing buffers
				if (e.GetKeyCode() == HELL_KEY_CAPS_LOCK)
					showBuffers = false;
			}
		}

		void ReCreateFBOs()
		{
			Application& app = Application::Get();
			GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
			glfwGetWindowSize(window, &SCR_WIDTH, &SCR_HEIGHT);

			gBuffer.Configure(SCR_WIDTH, SCR_HEIGHT);
			lightingBuffer.Configure(SCR_WIDTH, SCR_HEIGHT);

			blurBuffers[0].Configure(SCR_WIDTH / 2, SCR_HEIGHT / 2);
			blurBuffers[1].Configure(SCR_WIDTH / 4, SCR_HEIGHT / 4);
			blurBuffers[2].Configure(SCR_WIDTH / 8, SCR_HEIGHT / 8);
			blurBuffers[3].Configure(SCR_WIDTH / 16, SCR_HEIGHT / 16);
		}

		void DrawPoint(Shader *shader, glm::vec3 position, glm::mat4 modelMatrix, glm::vec3 color)
		{
			float vertices[] = { position.x, position.y, position.z };

			shader->setVec3("color", color);
			shader->setMat4("model", modelMatrix);

			unsigned int VAO, VBO;
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			glBindVertexArray(VAO);
			glDrawArrays(GL_POINTS, 0, 1);

			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		}		
		
		void DrawLine(Shader *shader, glm::vec3 pos1, glm::vec3 pos2, glm::mat4 modelMatrix, glm::vec3 color)
		{
			float vertices[] = { pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z };

			shader->setVec3("color", color);
			shader->setMat4("model", modelMatrix);

			unsigned int VAO, VBO;
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			glBindVertexArray(VAO);
			glDrawArrays(GL_LINES, 0, 2);

			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		}

		void AssingMousePickIDs()
		{
			mousePickIndices.clear();
			mousePickIndices.push_back(MousePickInfo{ MousePickType::NotFound, -1 }); // Prevent 0 being a colour, it's reserved for background.

			for (int i = -0; i < house.doors.size(); i++)
				mousePickIndices.push_back(MousePickInfo{ MousePickType::Door, i });

//			for (int i = -0; i < house.walls.size(); i++)
//				mousePickIndices.push_back(MousePickInfo{ MousePickType::Wall, i });
		}

		void Interact()
		{
			int i = mousePicked.indexInVector;
			MousePickType type = mousePicked.type;

			if (type == MousePickType::Door)
				house.doors.at(i).Interact();
		}

		MousePickInfo GetMousePicked()
		{			
			int i = GetPixelColorAtMouse();

			if (i >= 0 && i < mousePickIndices.size())
				return MousePickInfo{ mousePickIndices[i].type, mousePickIndices[i].indexInVector };
			else
				return MousePickInfo{ MousePickType::NotFound, -1 };
		}

		int GetPixelColorAtMouse()
		{
			// PBO stuff
			pBuffer.index = (pBuffer.index + 1) % 2;
			pBuffer.nextIndex = (pBuffer.index + 1) % 2;

			GLubyte* src;
			int result = -1;
			// READ PIXELS + 2 ALTERNATING PBOs
			if (true)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.ID);

				glBindBuffer(GL_PIXEL_PACK_BUFFER, pBuffer.pboIds[pBuffer.index]);
				glReadBuffer(GL_COLOR_ATTACHMENT3);
				glReadPixels(SCR_WIDTH / 2, SCR_HEIGHT / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);

				// map the PBO that contain framebuffer pixels before processing it
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pBuffer.pboIds[pBuffer.nextIndex]);
				GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
				if (src)
				{
					//result = *((int*)src); unsafe apparently
					result = *reinterpret_cast<int *>(src);
					glUnmapBuffer(GL_PIXEL_PACK_BUFFER); // // release pointer to the mapped buffer
				}
			}
			// GET TEXTURE SUB IMAGE
			/*if (false)
				glGetTextureSubImage(gBuffer.gMousePick, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2, 0, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 4, &pixelData);

			// PLANE OLD PLEB READ PIXELS
			if (false)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.ID);
				glReadBuffer(GL_COLOR_ATTACHMENT3);
				glReadPixels(SCR_WIDTH / 2, SCR_HEIGHT / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixelData);
			}*/
			return result;
		}

		void RebuildMap()
		{
			house.RebuildRooms();
			AssingMousePickIDs();
			boundingBoxPtrs = CreateBoudingBoxPtrsVector();
			boundingPlanePtrs = CreateBoudingPlanePtrsVector();
			_IMGUI_RUN_ONCE = true;
		}

		void ImguiFloat3(std::string text, glm::vec3* vector)
		{
			//static int imguiCounter = 0;
			//std::string id = "##" + std::to_string(imguiCounter++);
			std::string s = text + " [" + std::to_string(vector->x) + ", " + std::to_string(vector->y) + ", " + std::to_string(vector->z) + "]";
			ImGui::Text(s.c_str());// ImGui::SameLine();

			//glm::vec3* v;
			//ImGui::InputFloat3(id.c_str(), &vector, -10, 10);

		}

		RaycastData GetRaycastData(float variance)
		{
			 
			// RAYCASTING
			glm::vec3 ray_nds = glm::vec3(0, 0, 0);
			glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f);
			glm::vec4 ray_eye = glm::inverse(camera.projectionMatrix) * ray_clip;
			ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
			glm::vec3 r = glm::inverse(camera.viewMatrix) * ray_eye;
			ray_direction = glm::vec3(r.x, r.y, r.z);
			
			float offset = (variance * 0.5) - RandomFloat(0, variance);
			ray_direction.x += offset;
			offset = (variance * 0.5) - RandomFloat(0, variance);
			ray_direction.y += offset;
			offset = (variance * 0.5) - RandomFloat(0, variance);
			ray_direction.z += offset;

			ray_direction = glm::normalize(ray_direction);
			
			glm::vec3 ray_origin = camera.Position;

			RaycastData raycastData;
			raycastData.distance = 1000;
			raycastData.planeIndex = -1;
			raycastData.intersectionPoint = glm::vec3(0);

			for (int i = 0; i < boundingPlanePtrs.size(); i++)
			{
				BoundingPlane* plane = boundingPlanePtrs[i];
				// Check first triangle
				CollisionData collision = Util::RayTriangleIntersect(ray_origin, ray_direction, plane->A, plane->B, plane->C);
				if (collision.occured && collision.distance < raycastData.distance) {
					raycastData.intersectionPoint = collision.location;
					raycastData.distance = collision.distance;
					raycastData.planeIndex = i;
				}
				// Check second triangle
				collision = Util::RayTriangleIntersect(ray_origin, ray_direction, plane->C, plane->D, plane->A);
				if (collision.occured && collision.distance < raycastData.distance) {
					raycastData.intersectionPoint = collision.location;
					raycastData.distance = collision.distance;
					raycastData.planeIndex = i;
				}
			}
			return raycastData;
		}
	};


	class Sandbox : public HellEngine::Application
	{
	public:
		Sandbox()
		{
			PushLayer(new ExampleLayer());
		}

		~Sandbox()
		{
		 
		}

	};

	HellEngine::Application* HellEngine::CreateApplication()
	{
		return new Sandbox();
	}
}