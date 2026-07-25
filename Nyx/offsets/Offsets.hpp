#pragma once

#include <cstdint>
#include <string>
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <unordered_map>
#include <algorithm>

#pragma comment(lib, "wininet.lib")

namespace Offsets {

    inline std::string ClientVersion = "unknown";

    namespace AirProperties {
        inline uintptr_t AirDensity = 0x0;
        inline uintptr_t GlobalWind = 0x0;
    }

    namespace AnimationTrack {
        inline uintptr_t Animation = 0x0;
        inline uintptr_t Animator = 0x0;
        inline uintptr_t IsPlaying = 0x0;
        inline uintptr_t Looped = 0x0;
        inline uintptr_t Speed = 0x0;
        inline uintptr_t TimePosition = 0x0;
    }

    namespace Animator {
        inline uintptr_t ActiveAnimations = 0x0;
    }

    namespace Atmosphere {
        inline uintptr_t Color = 0x0;
        inline uintptr_t Decay = 0x0;
        inline uintptr_t Density = 0x0;
        inline uintptr_t Glare = 0x0;
        inline uintptr_t Haze = 0x0;
        inline uintptr_t Offset = 0x0;
    }

    namespace Attachment {
        inline uintptr_t Position = 0x0;
    }

    namespace Attribute {
        inline uintptr_t Key = 0x0;
        inline uintptr_t Size = 0x0;
        inline uintptr_t Value = 0x0;
    }

    namespace AttributesMap {
        inline uintptr_t Attributes = 0x0;
        inline uintptr_t Length = 0x0;
    }

    namespace BasePart {
        inline uintptr_t CastShadow = 0x0;
        inline uintptr_t Color3 = 0x0;
        inline uintptr_t Locked = 0x0;
        inline uintptr_t Massless = 0x0;
        inline uintptr_t Primitive = 0x0;
        inline uintptr_t Reflectance = 0x0;
        inline uintptr_t Shape = 0x0;
        inline uintptr_t Transparency = 0x0;
    }

    namespace Beam {
        inline uintptr_t Attachment0 = 0x0;
        inline uintptr_t Attachment1 = 0x0;
        inline uintptr_t Brightness = 0x0;
        inline uintptr_t CurveSize0 = 0x0;
        inline uintptr_t CurveSize1 = 0x0;
        inline uintptr_t LightEmission = 0x0;
        inline uintptr_t LightInfluence = 0x0;
        inline uintptr_t Texture = 0x0;
        inline uintptr_t TextureLength = 0x0;
        inline uintptr_t TextureSpeed = 0x0;
        inline uintptr_t Width0 = 0x0;
        inline uintptr_t Width1 = 0x0;
        inline uintptr_t ZOffset = 0x0;
    }

    namespace BloomEffect {
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t Intensity = 0x0;
        inline uintptr_t Size = 0x0;
        inline uintptr_t Threshold = 0x0;
    }

    namespace BlurEffect {
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t Size = 0x0;
    }

    namespace ByteCode {
        inline uintptr_t Pointer = 0x0;
        inline uintptr_t Size = 0x0;
    }

    namespace Camera {
        inline uintptr_t CameraSubject = 0x0;
        inline uintptr_t CameraType = 0x0;
        inline uintptr_t FieldOfView = 0x0;
        inline uintptr_t ImagePlaneDepth = 0x0;
        inline uintptr_t Position = 0x0;
        inline uintptr_t Rotation = 0x0;
        inline uintptr_t Viewport = 0x0;
        inline uintptr_t ViewportSize = 0x0;
    }

    namespace CharacterMesh {
        inline uintptr_t BaseTextureId = 0x0;
        inline uintptr_t BodyPart = 0x0;
        inline uintptr_t MeshId = 0x0;
        inline uintptr_t OverlayTextureId = 0x0;
    }

    namespace ClickDetector {
        inline uintptr_t MaxActivationDistance = 0x0;
        inline uintptr_t MouseIcon = 0x0;
    }

    namespace Clothing {
        inline uintptr_t Color3 = 0x0;
        inline uintptr_t Template = 0x0;
    }

    namespace ColorCorrectionEffect {
        inline uintptr_t Brightness = 0x0;
        inline uintptr_t Contrast = 0x0;
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t TintColor = 0x0;
    }

    namespace ColorGradingEffect {
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t TonemapperPreset = 0x0;
    }

    namespace DataModel {
        inline uintptr_t CreatorId = 0x0;
        inline uintptr_t GameId = 0x0;
        inline uintptr_t GameLoaded = 0x0;
        inline uintptr_t JobId = 0x0;
        inline uintptr_t PlaceId = 0x0;
        inline uintptr_t PlaceVersion = 0x0;
        inline uintptr_t PrimitiveCount = 0x0;
        inline uintptr_t ScriptContext = 0x0;
        inline uintptr_t ServerIP = 0x0;
        inline uintptr_t ToRenderView1 = 0x0;
        inline uintptr_t ToRenderView2 = 0x0;
        inline uintptr_t ToRenderView3 = 0x0;
        inline uintptr_t Workspace = 0x0;
    }

    namespace DepthOfFieldEffect {
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t FarIntensity = 0x0;
        inline uintptr_t FocusDistance = 0x0;
        inline uintptr_t InFocusRadius = 0x0;
        inline uintptr_t NearIntensity = 0x0;
    }

    namespace DragDetector {
        inline uintptr_t ActivatedCursorIcon = 0x0;
        inline uintptr_t CursorIcon = 0x0;
        inline uintptr_t MaxActivationDistance = 0x0;
        inline uintptr_t MaxDragAngle = 0x0;
        inline uintptr_t MaxDragTranslation = 0x0;
        inline uintptr_t MaxForce = 0x0;
        inline uintptr_t MaxTorque = 0x0;
        inline uintptr_t MinDragAngle = 0x0;
        inline uintptr_t MinDragTranslation = 0x0;
        inline uintptr_t ReferenceInstance = 0x0;
        inline uintptr_t Responsiveness = 0x0;
    }

    namespace FakeDataModel {
        inline uintptr_t Pointer = 0x0;
        inline uintptr_t RealDataModel = 0x0;
    }

    namespace GuiBase2D {
        inline uintptr_t AbsolutePosition = 0x0;
        inline uintptr_t AbsoluteRotation = 0x0;
        inline uintptr_t AbsoluteSize = 0x0;
    }

    namespace GuiObject {
        inline uintptr_t BackgroundColor3 = 0x0;
        inline uintptr_t BackgroundTransparency = 0x0;
        inline uintptr_t BorderColor3 = 0x0;
        inline uintptr_t Image = 0x0;
        inline uintptr_t LayoutOrder = 0x0;
        inline uintptr_t Position = 0x0;
        inline uintptr_t RichText = 0x0;
        inline uintptr_t Rotation = 0x0;
        inline uintptr_t ScreenGui_Enabled = 0x0;
        inline uintptr_t Size = 0x0;
        inline uintptr_t Text = 0x0;
        inline uintptr_t TextColor3 = 0x0;
        inline uintptr_t Visible = 0x0;
        inline uintptr_t ZIndex = 0x0;
    }

    namespace Humanoid {
        inline uintptr_t AutoJumpEnabled = 0x0;
        inline uintptr_t AutoRotate = 0x0;
        inline uintptr_t AutomaticScalingEnabled = 0x0;
        inline uintptr_t BreakJointsOnDeath = 0x0;
        inline uintptr_t CameraOffset = 0x0;
        inline uintptr_t DisplayDistanceType = 0x0;
        inline uintptr_t DisplayName = 0x0;
        inline uintptr_t EvaluateStateMachine = 0x0;
        inline uintptr_t FloorMaterial = 0x0;
        inline uintptr_t Health = 0x0;
        inline uintptr_t HealthDisplayDistance = 0x0;
        inline uintptr_t HealthDisplayType = 0x0;
        inline uintptr_t HipHeight = 0x0;
        inline uintptr_t HumanoidRootPart = 0x0;
        inline uintptr_t HumanoidState = 0x0;
        inline uintptr_t HumanoidStateID = 0x0;
        inline uintptr_t IsWalking = 0x0;
        inline uintptr_t Jump = 0x0;
        inline uintptr_t JumpHeight = 0x0;
        inline uintptr_t JumpPower = 0x0;
        inline uintptr_t MaxHealth = 0x0;
        inline uintptr_t MaxSlopeAngle = 0x0;
        inline uintptr_t MoveDirection = 0x0;
        inline uintptr_t MoveToPart = 0x0;
        inline uintptr_t MoveToPoint = 0x0;
        inline uintptr_t NameDisplayDistance = 0x0;
        inline uintptr_t NameOcclusion = 0x0;
        inline uintptr_t PlatformStand = 0x0;
        inline uintptr_t PlatformStatePointer = 0x0;
        inline uintptr_t RequiresNeck = 0x0;
        inline uintptr_t RigType = 0x0;
        inline uintptr_t SeatPart = 0x0;
        inline uintptr_t Sit = 0x0;
        inline uintptr_t TargetPoint = 0x0;
        inline uintptr_t UseJumpPower = 0x0;
        inline uintptr_t WalkTimer = 0x0;
        inline uintptr_t Walkspeed = 0x0;
        inline uintptr_t WalkspeedCheck = 0x0;
    }

    namespace Instance {
        inline uintptr_t ChildrenEnd = 0x0;
        inline uintptr_t ChildrenStart = 0x0;
        inline uintptr_t ClassBase = 0x0;
        inline uintptr_t ClassDescriptor = 0x0;
        inline uintptr_t ClassName = 0x0;
        inline uintptr_t ComponentMap = 0x0;
        inline uintptr_t Name = 0x0;
        inline uintptr_t Parent = 0x0;
        inline uintptr_t This = 0x0;
    }

    namespace Lighting {
        inline uintptr_t Ambient = 0x0;
        inline uintptr_t Brightness = 0x0;
        inline uintptr_t ClockTime = 0x0;
        inline uintptr_t ColorShift_Bottom = 0x0;
        inline uintptr_t ColorShift_Top = 0x0;
        inline uintptr_t EnvironmentDiffuseScale = 0x0;
        inline uintptr_t EnvironmentSpecularScale = 0x0;
        inline uintptr_t ExposureCompensation = 0x0;
        inline uintptr_t FogColor = 0x0;
        inline uintptr_t FogEnd = 0x0;
        inline uintptr_t FogStart = 0x0;
        inline uintptr_t GeographicLatitude = 0x0;
        inline uintptr_t GlobalShadows = 0x0;
        inline uintptr_t GradientBottom = 0x0;
        inline uintptr_t GradientTop = 0x0;
        inline uintptr_t LightColor = 0x0;
        inline uintptr_t LightDirection = 0x0;
        inline uintptr_t MoonPosition = 0x0;
        inline uintptr_t OutdoorAmbient = 0x0;
        inline uintptr_t Sky = 0x0;
        inline uintptr_t Source = 0x0;
        inline uintptr_t SunPosition = 0x0;
    }

    namespace LocalScript {
        inline uintptr_t ByteCode = 0x0;
        inline uintptr_t GUID = 0x0;
        inline uintptr_t Hash = 0x0;
    }

    namespace MaterialColors {
        inline uintptr_t Asphalt = 0x0;
        inline uintptr_t Basalt = 0x0;
        inline uintptr_t Brick = 0x0;
        inline uintptr_t Cobblestone = 0x0;
        inline uintptr_t Concrete = 0x0;
        inline uintptr_t CrackedLava = 0x0;
        inline uintptr_t Glacier = 0x0;
        inline uintptr_t Grass = 0x0;
        inline uintptr_t Ground = 0x0;
        inline uintptr_t Ice = 0x0;
        inline uintptr_t LeafyGrass = 0x0;
        inline uintptr_t Limestone = 0x0;
        inline uintptr_t Mud = 0x0;
        inline uintptr_t Pavement = 0x0;
        inline uintptr_t Rock = 0x0;
        inline uintptr_t Salt = 0x0;
        inline uintptr_t Sand = 0x0;
        inline uintptr_t Sandstone = 0x0;
        inline uintptr_t Slate = 0x0;
        inline uintptr_t Snow = 0x0;
        inline uintptr_t WoodPlanks = 0x0;
    }

    namespace MeshContentProvider {
        inline uintptr_t AssetID = 0x0;
        inline uintptr_t Cache = 0x0;
        inline uintptr_t LRUCache = 0x0;
        inline uintptr_t MeshData = 0x0;
        inline uintptr_t ToMeshData = 0x0;
    }

    namespace MeshData {
        inline uintptr_t FaceEnd = 0x0;
        inline uintptr_t FaceStart = 0x0;
        inline uintptr_t VertexEnd = 0x0;
        inline uintptr_t VertexStart = 0x0;
    }

    namespace MeshPart {
        inline uintptr_t MeshId = 0x0;
        inline uintptr_t Texture = 0x0;
    }

    namespace Misc {
        inline uintptr_t Adornee = 0x0;
        inline uintptr_t AnimationId = 0x0;
        inline uintptr_t StringLength = 0x0;
        inline uintptr_t Value = 0x0;
    }

    namespace Model {
        inline uintptr_t PrimaryPart = 0x0;
        inline uintptr_t Scale = 0x0;
    }

    namespace ModuleScript {
        inline uintptr_t ByteCode = 0x0;
        inline uintptr_t GUID = 0x0;
        inline uintptr_t Hash = 0x0;
        inline uintptr_t IsCoreScript = 0x0;
    }

    namespace MouseService {
        inline uintptr_t InputObject = 0x0;
        inline uintptr_t InputObject2 = 0x0;
        inline uintptr_t MousePosition = 0x0;
        inline uintptr_t SensitivityPointer = 0x0;
    }

    namespace ParticleEmitter {
        inline uintptr_t Acceleration = 0x0;
        inline uintptr_t Brightness = 0x0;
        inline uintptr_t Drag = 0x0;
        inline uintptr_t Lifetime = 0x0;
        inline uintptr_t LightEmission = 0x0;
        inline uintptr_t LightInfluence = 0x0;
        inline uintptr_t Rate = 0x0;
        inline uintptr_t RotSpeed = 0x0;
        inline uintptr_t Rotation = 0x0;
        inline uintptr_t Speed = 0x0;
        inline uintptr_t SpreadAngle = 0x0;
        inline uintptr_t Texture = 0x0;
        inline uintptr_t TimeScale = 0x0;
        inline uintptr_t VelocityInheritance = 0x0;
        inline uintptr_t ZOffset = 0x0;
    }

    namespace Player {
        inline uintptr_t AccountAge = 0x0;
        inline uintptr_t CameraMode = 0x0;
        inline uintptr_t DisplayName = 0x0;
        inline uintptr_t HealthDisplayDistance = 0x0;
        inline uintptr_t LocalPlayer = 0x0;
        inline uintptr_t LocaleId = 0x0;
        inline uintptr_t MaxZoomDistance = 0x0;
        inline uintptr_t MinZoomDistance = 0x0;
        inline uintptr_t ModelInstance = 0x0;
        inline uintptr_t Mouse = 0x0;
        inline uintptr_t NameDisplayDistance = 0x0;
        inline uintptr_t Team = 0x0;
        inline uintptr_t TeamColor = 0x0;
        inline uintptr_t UserId = 0x0;
    }

    namespace PlayerConfigurer {
        inline uintptr_t Pointer = 0x0;
    }

    namespace PlayerMouse {
        inline uintptr_t Icon = 0x0;
        inline uintptr_t Workspace = 0x0;
    }

    namespace Primitive {
        inline uintptr_t AssemblyAngularVelocity = 0x0;
        inline uintptr_t AssemblyLinearVelocity = 0x0;
        inline uintptr_t Flags = 0x0;
        inline uintptr_t Material = 0x0;
        inline uintptr_t Owner = 0x0;
        inline uintptr_t Position = 0x0;
        inline uintptr_t Rotation = 0x0;
        inline uintptr_t Size = 0x0;
        inline uintptr_t Validate = 0x0;
    }

    namespace PrimitiveFlags {
        inline uintptr_t Anchored = 0x0;
        inline uintptr_t CanCollide = 0x0;
        inline uintptr_t CanQuery = 0x0;
        inline uintptr_t CanTouch = 0x0;
    }

    namespace NextGenReplicator {
        inline uintptr_t EnabledWrite = 0x0;
    }

    namespace ProximityPrompt {
        inline uintptr_t ActionText = 0x0;
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t GamepadKeyCode = 0x0;
        inline uintptr_t HoldDuration = 0x0;
        inline uintptr_t KeyCode = 0x0;
        inline uintptr_t MaxActivationDistance = 0x0;
        inline uintptr_t ObjectText = 0x0;
        inline uintptr_t RequiresLineOfSight = 0x0;
    }

    namespace RenderJob {
        inline uintptr_t FakeDataModel = 0x0;
        inline uintptr_t RealDataModel = 0x0;
        inline uintptr_t RenderView = 0x0;
    }

    namespace RenderView {
        inline uintptr_t DeviceD3D11 = 0x0;
        inline uintptr_t LightingValid = 0x0;
        inline uintptr_t SkyValid = 0x0;
        inline uintptr_t VisualEngine = 0x0;
    }

    namespace RunService {
        inline uintptr_t HeartbeatFPS = 0x0;
        inline uintptr_t HeartbeatTask = 0x0;
    }

    namespace Script {
        inline uintptr_t ByteCode = 0x0;
        inline uintptr_t GUID = 0x0;
        inline uintptr_t Hash = 0x0;
    }

    namespace ScriptContext {
        inline uintptr_t RequireBypass = 0x0;
    }

    namespace Seat {
        inline uintptr_t Occupant = 0x0;
    }

    namespace Sky {
        inline uintptr_t MoonAngularSize = 0x0;
        inline uintptr_t MoonTextureId = 0x0;
        inline uintptr_t SkyboxBk = 0x0;
        inline uintptr_t SkyboxDn = 0x0;
        inline uintptr_t SkyboxFt = 0x0;
        inline uintptr_t SkyboxLf = 0x0;
        inline uintptr_t SkyboxOrientation = 0x0;
        inline uintptr_t SkyboxRt = 0x0;
        inline uintptr_t SkyboxUp = 0x0;
        inline uintptr_t StarCount = 0x0;
        inline uintptr_t SunAngularSize = 0x0;
        inline uintptr_t SunTextureId = 0x0;
    }

    namespace Sound {
        inline uintptr_t Looped = 0x0;
        inline uintptr_t PlaybackSpeed = 0x0;
        inline uintptr_t RollOffMaxDistance = 0x0;
        inline uintptr_t RollOffMinDistance = 0x0;
        inline uintptr_t SoundGroup = 0x0;
        inline uintptr_t SoundId = 0x0;
        inline uintptr_t Volume = 0x0;
    }

    namespace SpawnLocation {
        inline uintptr_t AllowTeamChangeOnTouch = 0x0;
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t ForcefieldDuration = 0x0;
        inline uintptr_t Neutral = 0x0;
        inline uintptr_t TeamColor = 0x0;
    }

    namespace SpecialMesh {
        inline uintptr_t MeshId = 0x0;
        inline uintptr_t Scale = 0x0;
    }

    namespace StatsItem {
        inline uintptr_t Value = 0x0;
    }

    namespace SunRaysEffect {
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t Intensity = 0x0;
        inline uintptr_t Spread = 0x0;
    }

    namespace SurfaceAppearance {
        inline uintptr_t AlphaMode = 0x0;
        inline uintptr_t Color = 0x0;
        inline uintptr_t ColorMap = 0x0;
        inline uintptr_t EmissiveMaskContent = 0x0;
        inline uintptr_t EmissiveStrength = 0x0;
        inline uintptr_t EmissiveTint = 0x0;
        inline uintptr_t MetalnessMap = 0x0;
        inline uintptr_t NormalMap = 0x0;
        inline uintptr_t RoughnessMap = 0x0;
    }

    namespace TaskScheduler {
        inline uintptr_t JobEnd = 0x0;
        inline uintptr_t JobName = 0x0;
        inline uintptr_t JobStart = 0x0;
        inline uintptr_t MaxFPS = 0x0;
        inline uintptr_t Pointer = 0x0;
    }

    namespace Team {
        inline uintptr_t BrickColor = 0x0;
    }

    namespace Terrain {
        inline uintptr_t GrassLength = 0x0;
        inline uintptr_t MaterialColors = 0x0;
        inline uintptr_t WaterColor = 0x0;
        inline uintptr_t WaterReflectance = 0x0;
        inline uintptr_t WaterTransparency = 0x0;
        inline uintptr_t WaterWaveSize = 0x0;
        inline uintptr_t WaterWaveSpeed = 0x0;
    }

    namespace Textures {
        inline uintptr_t Decal_Texture = 0x0;
        inline uintptr_t Texture_Texture = 0x0;
    }

    namespace Tool {
        inline uintptr_t CanBeDropped = 0x0;
        inline uintptr_t Enabled = 0x0;
        inline uintptr_t Grip = 0x0;
        inline uintptr_t ManualActivationOnly = 0x0;
        inline uintptr_t RequiresHandle = 0x0;
        inline uintptr_t TextureId = 0x0;
        inline uintptr_t Tooltip = 0x0;
    }

    namespace UnionOperation {
        inline uintptr_t AssetId = 0x0;
    }

    namespace UserInputService {
        inline uintptr_t WindowInputState = 0x0;
    }

    namespace VehicleSeat {
        inline uintptr_t MaxSpeed = 0x0;
        inline uintptr_t SteerFloat = 0x0;
        inline uintptr_t ThrottleFloat = 0x0;
        inline uintptr_t Torque = 0x0;
        inline uintptr_t TurnSpeed = 0x0;
    }

    namespace VisualEngine {
        inline uintptr_t Dimensions = 0x0;
        inline uintptr_t FakeDataModel = 0x0;
        inline uintptr_t Pointer = 0x0;
        inline uintptr_t RenderView = 0x0;
        inline uintptr_t ViewMatrix = 0x0;
    }

    namespace Weld {
        inline uintptr_t Part0 = 0x0;
        inline uintptr_t Part1 = 0x0;
    }

    namespace WeldConstraint {
        inline uintptr_t Part0 = 0x0;
        inline uintptr_t Part1 = 0x0;
    }

    namespace WindowInputState {
        inline uintptr_t CapsLock = 0x0;
        inline uintptr_t CurrentTextBox = 0x0;
    }

    namespace Workspace {
        inline uintptr_t CurrentCamera = 0x0;
        inline uintptr_t DistributedGameTime = 0x0;
        inline uintptr_t ReadOnlyGravity = 0x0;
        inline uintptr_t World = 0x0;
    }

    namespace World {
        inline uintptr_t AirProperties = 0x0;
        inline uintptr_t FallenPartsDestroyHeight = 0x0;
        inline uintptr_t Gravity = 0x0;
        inline uintptr_t Primitives = 0x0;
        inline uintptr_t worldStepsPerSec = 0x0;
    }

    inline std::string DownloadString(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("OffsetsFetcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string result;
        char buffer[4096];
        DWORD bytesRead;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return result;
    }

    inline uintptr_t ParseHex(const std::string& str) {
        if (str.find("0x") == 0 || str.find("0X") == 0) {
            return std::stoull(str.substr(2), nullptr, 16);
        }
        return std::stoull(str, nullptr, 16);
    }

    inline std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    inline bool FetchOffsets() {
        std::string content = DownloadString("https://offsets.imtheo.lol/offsets.hpp");
        if (content.empty()) return false;

        std::unordered_map<std::string, uintptr_t> offsetMap;
        std::string currentNamespace;
        std::istringstream stream(content);
        std::string line;

        while (std::getline(stream, line)) {

            if (line.find("inline std::string ClientVersion =") != std::string::npos) {
                size_t start = line.find("\"");
                if (start != std::string::npos) {
                    size_t end = line.find("\"", start + 1);
                    if (end != std::string::npos) {
                        ClientVersion = line.substr(start + 1, end - start - 1);
                    }
                }
                continue;
            }

            size_t nsPos = line.find("namespace ");
            if (nsPos != std::string::npos) {
                size_t start = nsPos + 10;
                size_t end = line.find(" {", start);
                if (end == std::string::npos) end = line.find("{", start);
                if (end != std::string::npos) {
                    currentNamespace = Trim(line.substr(start, end - start));
                }
                continue;
            }

            if (!currentNamespace.empty() && currentNamespace != "Offsets"
                && (line.find("inline constexpr uintptr_t") != std::string::npos
                    || line.find("inline uintptr_t") != std::string::npos))
            {
                size_t varStart = line.find("uintptr_t ");
                if (varStart == std::string::npos) continue;
                varStart += 10;
                size_t varEnd = line.find(" =", varStart);
                if (varEnd == std::string::npos) varEnd = line.find('=', varStart);
                if (varEnd == std::string::npos) continue;

                std::string varName = Trim(line.substr(varStart, varEnd - varStart));
                size_t hexStart = line.find("0x", varEnd);
                if (hexStart == std::string::npos) continue;
                size_t hexEnd = line.find(';', hexStart);
                if (hexEnd == std::string::npos) continue;

                std::string hexValue = Trim(line.substr(hexStart, hexEnd - hexStart));
                offsetMap[currentNamespace + "::" + varName] = ParseHex(hexValue);
            }

            if (line.find("}") != std::string::npos && !currentNamespace.empty()) {

                if (line.find("namespace") == std::string::npos) {

                }
            }
        }

#define SET_OFFSET(ns, var) if (offsetMap.find(#ns "::" #var) != offsetMap.end()) ns::var = offsetMap[#ns "::" #var];

        SET_OFFSET(AirProperties, AirDensity);
        SET_OFFSET(AirProperties, GlobalWind);

        SET_OFFSET(AnimationTrack, Animation);
        SET_OFFSET(AnimationTrack, Animator);
        SET_OFFSET(AnimationTrack, IsPlaying);
        SET_OFFSET(AnimationTrack, Looped);
        SET_OFFSET(AnimationTrack, Speed);
        SET_OFFSET(AnimationTrack, TimePosition);

        SET_OFFSET(Animator, ActiveAnimations);

        SET_OFFSET(Atmosphere, Color);
        SET_OFFSET(Atmosphere, Decay);
        SET_OFFSET(Atmosphere, Density);
        SET_OFFSET(Atmosphere, Glare);
        SET_OFFSET(Atmosphere, Haze);
        SET_OFFSET(Atmosphere, Offset);

        SET_OFFSET(Attachment, Position);

        SET_OFFSET(Attribute, Key);
        SET_OFFSET(Attribute, Size);
        SET_OFFSET(Attribute, Value);

        SET_OFFSET(AttributesMap, Attributes);
        SET_OFFSET(AttributesMap, Length);

        SET_OFFSET(BasePart, CastShadow);
        SET_OFFSET(BasePart, Color3);
        SET_OFFSET(BasePart, Locked);
        SET_OFFSET(BasePart, Massless);
        SET_OFFSET(BasePart, Primitive);
        SET_OFFSET(BasePart, Reflectance);
        SET_OFFSET(BasePart, Shape);
        SET_OFFSET(BasePart, Transparency);

        SET_OFFSET(Beam, Attachment0);
        SET_OFFSET(Beam, Attachment1);
        SET_OFFSET(Beam, Brightness);
        SET_OFFSET(Beam, CurveSize0);
        SET_OFFSET(Beam, CurveSize1);
        SET_OFFSET(Beam, LightEmission);
        SET_OFFSET(Beam, LightInfluence);
        SET_OFFSET(Beam, Texture);
        SET_OFFSET(Beam, TextureLength);
        SET_OFFSET(Beam, TextureSpeed);
        SET_OFFSET(Beam, Width0);
        SET_OFFSET(Beam, Width1);
        SET_OFFSET(Beam, ZOffset);

        SET_OFFSET(BloomEffect, Enabled);
        SET_OFFSET(BloomEffect, Intensity);
        SET_OFFSET(BloomEffect, Size);
        SET_OFFSET(BloomEffect, Threshold);

        SET_OFFSET(BlurEffect, Enabled);
        SET_OFFSET(BlurEffect, Size);

        SET_OFFSET(ByteCode, Pointer);
        SET_OFFSET(ByteCode, Size);

        SET_OFFSET(Camera, CameraSubject);
        SET_OFFSET(Camera, CameraType);
        SET_OFFSET(Camera, FieldOfView);
        SET_OFFSET(Camera, ImagePlaneDepth);
        SET_OFFSET(Camera, Position);
        SET_OFFSET(Camera, Rotation);
        SET_OFFSET(Camera, Viewport);
        SET_OFFSET(Camera, ViewportSize);

        SET_OFFSET(CharacterMesh, BaseTextureId);
        SET_OFFSET(CharacterMesh, BodyPart);
        SET_OFFSET(CharacterMesh, MeshId);
        SET_OFFSET(CharacterMesh, OverlayTextureId);

        SET_OFFSET(ClickDetector, MaxActivationDistance);
        SET_OFFSET(ClickDetector, MouseIcon);

        SET_OFFSET(Clothing, Color3);
        SET_OFFSET(Clothing, Template);

        SET_OFFSET(ColorCorrectionEffect, Brightness);
        SET_OFFSET(ColorCorrectionEffect, Contrast);
        SET_OFFSET(ColorCorrectionEffect, Enabled);
        SET_OFFSET(ColorCorrectionEffect, TintColor);

        SET_OFFSET(ColorGradingEffect, Enabled);
        SET_OFFSET(ColorGradingEffect, TonemapperPreset);

        SET_OFFSET(DataModel, CreatorId);
        SET_OFFSET(DataModel, GameId);
        SET_OFFSET(DataModel, GameLoaded);
        SET_OFFSET(DataModel, JobId);
        SET_OFFSET(DataModel, PlaceId);
        SET_OFFSET(DataModel, PlaceVersion);
        SET_OFFSET(DataModel, PrimitiveCount);
        SET_OFFSET(DataModel, ScriptContext);
        SET_OFFSET(DataModel, ServerIP);
        SET_OFFSET(DataModel, ToRenderView1);
        SET_OFFSET(DataModel, ToRenderView2);
        SET_OFFSET(DataModel, ToRenderView3);
        SET_OFFSET(DataModel, Workspace);

        SET_OFFSET(DepthOfFieldEffect, Enabled);
        SET_OFFSET(DepthOfFieldEffect, FarIntensity);
        SET_OFFSET(DepthOfFieldEffect, FocusDistance);
        SET_OFFSET(DepthOfFieldEffect, InFocusRadius);
        SET_OFFSET(DepthOfFieldEffect, NearIntensity);

        SET_OFFSET(DragDetector, ActivatedCursorIcon);
        SET_OFFSET(DragDetector, CursorIcon);
        SET_OFFSET(DragDetector, MaxActivationDistance);
        SET_OFFSET(DragDetector, MaxDragAngle);
        SET_OFFSET(DragDetector, MaxDragTranslation);
        SET_OFFSET(DragDetector, MaxForce);
        SET_OFFSET(DragDetector, MaxTorque);
        SET_OFFSET(DragDetector, MinDragAngle);
        SET_OFFSET(DragDetector, MinDragTranslation);
        SET_OFFSET(DragDetector, ReferenceInstance);
        SET_OFFSET(DragDetector, Responsiveness);

        SET_OFFSET(FakeDataModel, Pointer);
        SET_OFFSET(FakeDataModel, RealDataModel);

        SET_OFFSET(GuiBase2D, AbsolutePosition);
        SET_OFFSET(GuiBase2D, AbsoluteRotation);
        SET_OFFSET(GuiBase2D, AbsoluteSize);

        SET_OFFSET(GuiObject, BackgroundColor3);
        SET_OFFSET(GuiObject, BackgroundTransparency);
        SET_OFFSET(GuiObject, BorderColor3);
        SET_OFFSET(GuiObject, Image);
        SET_OFFSET(GuiObject, LayoutOrder);
        SET_OFFSET(GuiObject, Position);
        SET_OFFSET(GuiObject, RichText);
        SET_OFFSET(GuiObject, Rotation);
        SET_OFFSET(GuiObject, ScreenGui_Enabled);
        SET_OFFSET(GuiObject, Size);
        SET_OFFSET(GuiObject, Text);
        SET_OFFSET(GuiObject, TextColor3);
        SET_OFFSET(GuiObject, Visible);
        SET_OFFSET(GuiObject, ZIndex);

        SET_OFFSET(Humanoid, AutoJumpEnabled);
        SET_OFFSET(Humanoid, AutoRotate);
        SET_OFFSET(Humanoid, AutomaticScalingEnabled);
        SET_OFFSET(Humanoid, BreakJointsOnDeath);
        SET_OFFSET(Humanoid, CameraOffset);
        SET_OFFSET(Humanoid, DisplayDistanceType);
        SET_OFFSET(Humanoid, DisplayName);
        SET_OFFSET(Humanoid, EvaluateStateMachine);
        SET_OFFSET(Humanoid, FloorMaterial);
        SET_OFFSET(Humanoid, Health);
        SET_OFFSET(Humanoid, HealthDisplayDistance);
        SET_OFFSET(Humanoid, HealthDisplayType);
        SET_OFFSET(Humanoid, HipHeight);
        SET_OFFSET(Humanoid, HumanoidRootPart);
        SET_OFFSET(Humanoid, HumanoidState);
        SET_OFFSET(Humanoid, HumanoidStateID);
        SET_OFFSET(Humanoid, IsWalking);
        SET_OFFSET(Humanoid, Jump);
        SET_OFFSET(Humanoid, JumpHeight);
        SET_OFFSET(Humanoid, JumpPower);
        SET_OFFSET(Humanoid, MaxHealth);
        SET_OFFSET(Humanoid, MaxSlopeAngle);
        SET_OFFSET(Humanoid, MoveDirection);
        SET_OFFSET(Humanoid, MoveToPart);
        SET_OFFSET(Humanoid, MoveToPoint);
        SET_OFFSET(Humanoid, NameDisplayDistance);
        SET_OFFSET(Humanoid, NameOcclusion);
        SET_OFFSET(Humanoid, PlatformStand);
        SET_OFFSET(Humanoid, PlatformStatePointer);
        SET_OFFSET(Humanoid, RequiresNeck);
        SET_OFFSET(Humanoid, RigType);
        SET_OFFSET(Humanoid, SeatPart);
        SET_OFFSET(Humanoid, Sit);
        SET_OFFSET(Humanoid, TargetPoint);
        SET_OFFSET(Humanoid, UseJumpPower);
        SET_OFFSET(Humanoid, WalkTimer);
        SET_OFFSET(Humanoid, Walkspeed);
        SET_OFFSET(Humanoid, WalkspeedCheck);

        SET_OFFSET(Instance, ChildrenEnd);
        SET_OFFSET(Instance, ChildrenStart);
        SET_OFFSET(Instance, ClassBase);
        SET_OFFSET(Instance, ClassDescriptor);
        SET_OFFSET(Instance, ClassName);
        SET_OFFSET(Instance, ComponentMap);
        SET_OFFSET(Instance, Name);
        SET_OFFSET(Instance, Parent);
        SET_OFFSET(Instance, This);

        SET_OFFSET(Lighting, Ambient);
        SET_OFFSET(Lighting, Brightness);
        SET_OFFSET(Lighting, ClockTime);
        SET_OFFSET(Lighting, ColorShift_Bottom);
        SET_OFFSET(Lighting, ColorShift_Top);
        SET_OFFSET(Lighting, EnvironmentDiffuseScale);
        SET_OFFSET(Lighting, EnvironmentSpecularScale);
        SET_OFFSET(Lighting, ExposureCompensation);
        SET_OFFSET(Lighting, FogColor);
        SET_OFFSET(Lighting, FogEnd);
        SET_OFFSET(Lighting, FogStart);
        SET_OFFSET(Lighting, GeographicLatitude);
        SET_OFFSET(Lighting, GlobalShadows);
        SET_OFFSET(Lighting, GradientBottom);
        SET_OFFSET(Lighting, GradientTop);
        SET_OFFSET(Lighting, LightColor);
        SET_OFFSET(Lighting, LightDirection);
        SET_OFFSET(Lighting, MoonPosition);
        SET_OFFSET(Lighting, OutdoorAmbient);
        SET_OFFSET(Lighting, Sky);
        SET_OFFSET(Lighting, Source);
        SET_OFFSET(Lighting, SunPosition);

        SET_OFFSET(LocalScript, ByteCode);
        SET_OFFSET(LocalScript, GUID);
        SET_OFFSET(LocalScript, Hash);

        SET_OFFSET(MaterialColors, Asphalt);
        SET_OFFSET(MaterialColors, Basalt);
        SET_OFFSET(MaterialColors, Brick);
        SET_OFFSET(MaterialColors, Cobblestone);
        SET_OFFSET(MaterialColors, Concrete);
        SET_OFFSET(MaterialColors, CrackedLava);
        SET_OFFSET(MaterialColors, Glacier);
        SET_OFFSET(MaterialColors, Grass);
        SET_OFFSET(MaterialColors, Ground);
        SET_OFFSET(MaterialColors, Ice);
        SET_OFFSET(MaterialColors, LeafyGrass);
        SET_OFFSET(MaterialColors, Limestone);
        SET_OFFSET(MaterialColors, Mud);
        SET_OFFSET(MaterialColors, Pavement);
        SET_OFFSET(MaterialColors, Rock);
        SET_OFFSET(MaterialColors, Salt);
        SET_OFFSET(MaterialColors, Sand);
        SET_OFFSET(MaterialColors, Sandstone);
        SET_OFFSET(MaterialColors, Slate);
        SET_OFFSET(MaterialColors, Snow);
        SET_OFFSET(MaterialColors, WoodPlanks);

        SET_OFFSET(MeshContentProvider, AssetID);
        SET_OFFSET(MeshContentProvider, Cache);
        SET_OFFSET(MeshContentProvider, LRUCache);
        SET_OFFSET(MeshContentProvider, MeshData);
        SET_OFFSET(MeshContentProvider, ToMeshData);

        SET_OFFSET(MeshData, FaceEnd);
        SET_OFFSET(MeshData, FaceStart);
        SET_OFFSET(MeshData, VertexEnd);
        SET_OFFSET(MeshData, VertexStart);

        SET_OFFSET(MeshPart, MeshId);
        SET_OFFSET(MeshPart, Texture);

        SET_OFFSET(Misc, Adornee);
        SET_OFFSET(Misc, AnimationId);
        SET_OFFSET(Misc, StringLength);
        SET_OFFSET(Misc, Value);

        SET_OFFSET(Model, PrimaryPart);
        SET_OFFSET(Model, Scale);

        SET_OFFSET(ModuleScript, ByteCode);
        SET_OFFSET(ModuleScript, GUID);
        SET_OFFSET(ModuleScript, Hash);
        SET_OFFSET(ModuleScript, IsCoreScript);

        SET_OFFSET(MouseService, InputObject);
        SET_OFFSET(MouseService, InputObject2);
        SET_OFFSET(MouseService, MousePosition);
        SET_OFFSET(MouseService, SensitivityPointer);

        SET_OFFSET(ParticleEmitter, Acceleration);
        SET_OFFSET(ParticleEmitter, Brightness);
        SET_OFFSET(ParticleEmitter, Drag);
        SET_OFFSET(ParticleEmitter, Lifetime);
        SET_OFFSET(ParticleEmitter, LightEmission);
        SET_OFFSET(ParticleEmitter, LightInfluence);
        SET_OFFSET(ParticleEmitter, Rate);
        SET_OFFSET(ParticleEmitter, RotSpeed);
        SET_OFFSET(ParticleEmitter, Rotation);
        SET_OFFSET(ParticleEmitter, Speed);
        SET_OFFSET(ParticleEmitter, SpreadAngle);
        SET_OFFSET(ParticleEmitter, Texture);
        SET_OFFSET(ParticleEmitter, TimeScale);
        SET_OFFSET(ParticleEmitter, VelocityInheritance);
        SET_OFFSET(ParticleEmitter, ZOffset);

        SET_OFFSET(Player, AccountAge);
        SET_OFFSET(Player, CameraMode);
        SET_OFFSET(Player, DisplayName);
        SET_OFFSET(Player, HealthDisplayDistance);
        SET_OFFSET(Player, LocalPlayer);
        SET_OFFSET(Player, LocaleId);
        SET_OFFSET(Player, MaxZoomDistance);
        SET_OFFSET(Player, MinZoomDistance);
        SET_OFFSET(Player, ModelInstance);
        SET_OFFSET(Player, Mouse);
        SET_OFFSET(Player, NameDisplayDistance);
        SET_OFFSET(Player, Team);
        SET_OFFSET(Player, TeamColor);
        SET_OFFSET(Player, UserId);

        SET_OFFSET(PlayerConfigurer, Pointer);

        SET_OFFSET(PlayerMouse, Icon);
        SET_OFFSET(PlayerMouse, Workspace);

        SET_OFFSET(Primitive, AssemblyAngularVelocity);
        SET_OFFSET(Primitive, AssemblyLinearVelocity);
        SET_OFFSET(Primitive, Flags);
        SET_OFFSET(Primitive, Material);
        SET_OFFSET(Primitive, Owner);
        SET_OFFSET(Primitive, Position);
        SET_OFFSET(Primitive, Rotation);
        SET_OFFSET(Primitive, Size);
        SET_OFFSET(Primitive, Validate);

        SET_OFFSET(PrimitiveFlags, Anchored);
        SET_OFFSET(PrimitiveFlags, CanCollide);
        SET_OFFSET(PrimitiveFlags, CanQuery);
        SET_OFFSET(PrimitiveFlags, CanTouch);

        SET_OFFSET(ProximityPrompt, ActionText);
        SET_OFFSET(ProximityPrompt, Enabled);
        SET_OFFSET(ProximityPrompt, GamepadKeyCode);
        SET_OFFSET(ProximityPrompt, HoldDuration);
        SET_OFFSET(ProximityPrompt, KeyCode);
        SET_OFFSET(ProximityPrompt, MaxActivationDistance);
        SET_OFFSET(ProximityPrompt, ObjectText);
        SET_OFFSET(ProximityPrompt, RequiresLineOfSight);

        SET_OFFSET(RenderJob, FakeDataModel);
        SET_OFFSET(RenderJob, RealDataModel);
        SET_OFFSET(RenderJob, RenderView);

        SET_OFFSET(RenderView, DeviceD3D11);
        SET_OFFSET(RenderView, LightingValid);
        SET_OFFSET(RenderView, SkyValid);
        SET_OFFSET(RenderView, VisualEngine);

        SET_OFFSET(RunService, HeartbeatFPS);
        SET_OFFSET(RunService, HeartbeatTask);

        SET_OFFSET(Script, ByteCode);
        SET_OFFSET(Script, GUID);
        SET_OFFSET(Script, Hash);

        SET_OFFSET(ScriptContext, RequireBypass);

        SET_OFFSET(Seat, Occupant);

        SET_OFFSET(Sky, MoonAngularSize);
        SET_OFFSET(Sky, MoonTextureId);
        SET_OFFSET(Sky, SkyboxBk);
        SET_OFFSET(Sky, SkyboxDn);
        SET_OFFSET(Sky, SkyboxFt);
        SET_OFFSET(Sky, SkyboxLf);
        SET_OFFSET(Sky, SkyboxOrientation);
        SET_OFFSET(Sky, SkyboxRt);
        SET_OFFSET(Sky, SkyboxUp);
        SET_OFFSET(Sky, StarCount);
        SET_OFFSET(Sky, SunAngularSize);
        SET_OFFSET(Sky, SunTextureId);

        SET_OFFSET(Sound, Looped);
        SET_OFFSET(Sound, PlaybackSpeed);
        SET_OFFSET(Sound, RollOffMaxDistance);
        SET_OFFSET(Sound, RollOffMinDistance);
        SET_OFFSET(Sound, SoundGroup);
        SET_OFFSET(Sound, SoundId);
        SET_OFFSET(Sound, Volume);

        SET_OFFSET(SpawnLocation, AllowTeamChangeOnTouch);
        SET_OFFSET(SpawnLocation, Enabled);
        SET_OFFSET(SpawnLocation, ForcefieldDuration);
        SET_OFFSET(SpawnLocation, Neutral);
        SET_OFFSET(SpawnLocation, TeamColor);

        SET_OFFSET(SpecialMesh, MeshId);
        SET_OFFSET(SpecialMesh, Scale);

        SET_OFFSET(StatsItem, Value);

        SET_OFFSET(SunRaysEffect, Enabled);
        SET_OFFSET(SunRaysEffect, Intensity);
        SET_OFFSET(SunRaysEffect, Spread);

        SET_OFFSET(SurfaceAppearance, AlphaMode);
        SET_OFFSET(SurfaceAppearance, Color);
        SET_OFFSET(SurfaceAppearance, ColorMap);
        SET_OFFSET(SurfaceAppearance, EmissiveMaskContent);
        SET_OFFSET(SurfaceAppearance, EmissiveStrength);
        SET_OFFSET(SurfaceAppearance, EmissiveTint);
        SET_OFFSET(SurfaceAppearance, MetalnessMap);
        SET_OFFSET(SurfaceAppearance, NormalMap);
        SET_OFFSET(SurfaceAppearance, RoughnessMap);

        SET_OFFSET(TaskScheduler, JobEnd);
        SET_OFFSET(TaskScheduler, JobName);
        SET_OFFSET(TaskScheduler, JobStart);
        SET_OFFSET(TaskScheduler, MaxFPS);
        SET_OFFSET(TaskScheduler, Pointer);

        SET_OFFSET(Team, BrickColor);

        SET_OFFSET(Terrain, GrassLength);
        SET_OFFSET(Terrain, MaterialColors);
        SET_OFFSET(Terrain, WaterColor);
        SET_OFFSET(Terrain, WaterReflectance);
        SET_OFFSET(Terrain, WaterTransparency);
        SET_OFFSET(Terrain, WaterWaveSize);
        SET_OFFSET(Terrain, WaterWaveSpeed);

        SET_OFFSET(Textures, Decal_Texture);
        SET_OFFSET(Textures, Texture_Texture);

        SET_OFFSET(Tool, CanBeDropped);
        SET_OFFSET(Tool, Enabled);
        SET_OFFSET(Tool, Grip);
        SET_OFFSET(Tool, ManualActivationOnly);
        SET_OFFSET(Tool, RequiresHandle);
        SET_OFFSET(Tool, TextureId);
        SET_OFFSET(Tool, Tooltip);

        SET_OFFSET(UnionOperation, AssetId);

        SET_OFFSET(UserInputService, WindowInputState);

        SET_OFFSET(VehicleSeat, MaxSpeed);
        SET_OFFSET(VehicleSeat, SteerFloat);
        SET_OFFSET(VehicleSeat, ThrottleFloat);
        SET_OFFSET(VehicleSeat, Torque);
        SET_OFFSET(VehicleSeat, TurnSpeed);

        SET_OFFSET(VisualEngine, Dimensions);
        SET_OFFSET(VisualEngine, FakeDataModel);
        SET_OFFSET(VisualEngine, Pointer);
        SET_OFFSET(VisualEngine, RenderView);
        SET_OFFSET(VisualEngine, ViewMatrix);

        SET_OFFSET(Weld, Part0);
        SET_OFFSET(Weld, Part1);

        SET_OFFSET(WeldConstraint, Part0);
        SET_OFFSET(WeldConstraint, Part1);

        SET_OFFSET(WindowInputState, CapsLock);
        SET_OFFSET(WindowInputState, CurrentTextBox);

        SET_OFFSET(Workspace, CurrentCamera);
        SET_OFFSET(Workspace, DistributedGameTime);
        SET_OFFSET(Workspace, ReadOnlyGravity);
        SET_OFFSET(Workspace, World);

        SET_OFFSET(World, AirProperties);
        SET_OFFSET(World, FallenPartsDestroyHeight);
        SET_OFFSET(World, Gravity);
        SET_OFFSET(World, Primitives);
        SET_OFFSET(World, worldStepsPerSec);

        SET_OFFSET(NextGenReplicator, EnabledWrite);

#undef SET_OFFSET

        return true;
    }
}
