
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodyInstance.h"

#include "QFMTypes.h"
#include "QFMPIDController.h"

#include "QFMInputController.h"
#include "QFMAHRS.h"
#include "QFMPositionController.h"
#include "QFMEngineController.h"

#include "QFMAttitudeController.generated.h"


/*--- Implementatrion of the Attitude and Position Flight-Controller ---*/
USTRUCT(BlueprintType)
struct FAttitudeController
{
	GENERATED_BODY()


	/*--- PARAMETERS ---*/

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "FlightMode")) 
	EFlightMode FlightMode = EFlightMode::FM_Stabilize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Max Lean Angle Deg in Stab Mode")) 
	float AngleMax = 45.0f;  
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Smoothing Value for StabilizerMode. Must be 0..1")) 
	float SmoothingGain = 0.5f;  
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Zero Pos on Throttle for Accro Mode. Must be 0..1")) 
	float AccroThrottleMid = 0.5f;   
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Max Speed Down in m/s in Stab/Accro Mode")) 
	float PilotSpeedDown = 20.0f;   
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Max Speed Up in m/s in Stab/Accro Mode")) 
	float PilotSpeedUp = 20.0f;   
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "1..10 , 4.5 = 200 (deg/s) Max rotation rate of yaw axis")) 
	float YawPGain = 200.0f;   
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "1..10 , 4.5 = 200 (deg/s) Max rotation rate of roll/pitch axis")) 
	float AccroRollPitchPGain = 200.0f;   
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "-0.5..1 Amount of Expo to add to Accro Yaw 0 = disable, 1.0 = very high")) 
	float AccroYawExpo = 0.0f;   
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "-0.5..1 Amount of Expo to add to Accro Yaw 0 = disable, 1.0 = very high")) 
	float AccroRollPitchExpo = 0.0f;  
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "deadzone in % (0..1) up and % down from center")) 
	float ThrottleDeadzone = 0.1f;   

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Stabilizer-Loop to use for Rotations")) 
	EControlLoop RotationControlLoop = EControlLoop::ControlLoop_P;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Roll Rate PID"))
	FVector RateRollPidSettings = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Pitch Rate PID"))
	FVector RatePitchPidSettings = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "Yaw Rate PID"))
	FVector RateYawPidSettings = FVector(0.0f, 0.0f, 0.0f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "FPD Damping. 1=crit damped, <1 = underdamped, >1 = overdamped"))
	float SPDDamping = 1.0f;	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuadcopterFlightModel", meta = (ToolTip = "FPD Frequency. Reach 95% of target in 1/Freq secs"))
	float SPDFrequency = 0.1f;

	
	/*--- PRIVATE ---*/

	// Targets
	UPROPERTY() FQuat AttitudeTargetQuat;

	// PIDs
	UPROPERTY() FPIDController RateRollPid;
	UPROPERTY() FPIDController RatePitchPid;
	UPROPERTY() FPIDController RateYawPid;


	/*--- INTERFACE DATA ---*/
	// Copy of Parent Data. Put inside here during Tock or Init
	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	FVector4 PilotInput;

	FBodyInstance *BodyInstance;
	UPrimitiveComponent *PrimitiveComponent;
	
	FAHRS *AHRS;
	FPositionController *PositionController;
	FEngineController *EngineController;
	FInputController *InputController;
	
	/*--- UDP PID DEBUG ---*/
	FVector UDPDebugOutput;

	FVector GetUDPDebugOutput()
	{
		return UDPDebugOutput;
	}


	/*--- INIT FLIGHT MODES ---*/

	void Init(FBodyInstance *BodyInstanceIn, UPrimitiveComponent *PrimitiveComponentIn, FInputController *InputControllerIn, FAHRS *AHRSIn, FPositionController *PositionControllerIn, FEngineController *EngineControllerIn)
	{

		// Set up UDP Debug Output
		UDPDebugOutput = FVector::ZeroVector;

		// Set up Interface
	
		BodyInstance = BodyInstanceIn;
		PrimitiveComponent = PrimitiveComponentIn;
		InputController = InputControllerIn;
		AHRS = AHRSIn;
		PositionController = PositionControllerIn;
		EngineController = EngineControllerIn;

		// Init Pids with min,max = -1..1. We normalize velocities in RunQuat(). So we allways have the same PID-Settinghs, regardeless of Max Rates
		RateRollPid.Init(-1, 1, RateRollPidSettings.X, RateRollPidSettings.Y, RateRollPidSettings.Z);
		RatePitchPid.Init(-1, 1, RatePitchPidSettings.X, RatePitchPidSettings.Y, RatePitchPidSettings.Z);
		RateYawPid.Init(-1, 1, RateYawPidSettings.X, RateYawPidSettings.Y, RateYawPidSettings.Z);

		Reset();

		// we start in Stabilize mode
		SelectFlightMode(FlightMode);
	}


	void Reset()
	{
		// Reset Quats
		FTransform bodyTransform =  BodyInstance->GetUnrealWorldTransform();
		AttitudeTargetQuat = bodyTransform.GetRotation();
	
		// ResetPids
		RateRollPid.Reset();
		RatePitchPid.Reset();
		RateYawPid.Reset();
	}


	void SelectFlightMode(EFlightMode FlightModeIn)
	{
		FlightMode = FlightModeIn;

		switch (FlightMode)
		{
		case EFlightMode::FM_Direct:
			InitModeDirect();
				break;
		case EFlightMode::FM_Stabilize:
			InitModeStabilize();
				break;
		case EFlightMode::FM_AltHold:
			InitModeAltHold();
				break;
		case EFlightMode::FM_Accro:
			InitModeAccro();
				break;
		default:
			break;
		}
	}


	void InitModeDirect()
	{
		// Nothing to do here
	}

	void InitModeStabilize()
	{
		PositionController->SetAltTarget(0.0f);
	}

	void InitModeAltHold()
	{
		if (!PositionController->IsActiveZ())
		{
			PositionController->SetAltTargetToCurrentAlt();
		}
	}

	void InitModeAccro()
	{
		PositionController->SetAltTarget(0.0f);
	}


	/*--- Tock Method. Call this from parents Tick() ---*/

	void Tock(float DeltaTimeIn)
	{
		DeltaTime = DeltaTimeIn;
		PilotInput = InputController->GetDesiredInput();

		switch (FlightMode)
		{
		case EFlightMode::FM_Direct:
			TockModeDirect();
				break;
		case EFlightMode::FM_Stabilize:
			TockModeStabilize();
				break;
		case EFlightMode::FM_AltHold:
			TockModeAltHold();
				break;
		case EFlightMode::FM_Accro:
			TockModeAccro();
				break;
		default:
			break;
		}
	}


	void TockModeDirect()
	{
		float ThrottleScaled;
		ThrottleScaled = GetPilotDesiredThrottle(PilotInput.W);
		EngineController->SetDesiredThrottlePercent(ThrottleScaled);

		FTransform bodyTransform =  BodyInstance->GetUnrealWorldTransform();
		FVector AngularVelocityToApply = FVector(
			PilotInput.X,
			PilotInput.Y,
			PilotInput.Z
		);
		EngineController->SetDesiredRotationForces(
			AngularVelocityToApply //* DeltaTime
		);
	}


	void TockModeStabilize()
	{
		float TargetRoll;
		float TargetPitch;
		float TargetYawRate;
		float ThrottleScaled;

		GetPilotDesiredLeanAngles(PilotInput.X, PilotInput.Y, TargetRoll, TargetPitch);
		TargetYawRate = GetPilotDesiredYawRate(PilotInput.Z);
		ThrottleScaled = GetPilotDesiredThrottle(PilotInput.W);

		InputAngleRollPitchRateYaw(TargetRoll, TargetPitch, TargetYawRate);
		EngineController->SetDesiredThrottlePercent(ThrottleScaled);
	}


	void TockModeAltHold()
	{

		float TakeoffClimbRate = 0.0f;

		float TargetRoll;
		float TargetPitch;
		float TargetYawRate;
		float TargetClimbRate;

		GetPilotDesiredLeanAngles(PilotInput.X, PilotInput.Y, TargetRoll, TargetPitch);
		TargetYawRate = GetPilotDesiredYawRate(PilotInput.Z);
		TargetClimbRate = GetPilotDesiredClimbRate(PilotInput.W);
		
		InputAngleRollPitchRateYaw(TargetRoll, TargetPitch, TargetYawRate);
		PositionController->SetAltTargetFromClimbRate(TargetClimbRate);
		PositionController->UpdateZController();
	}


	void TockModeAccro()
	{
		float TargetRollRate;
		float TargetPitchRate;
		float TargetYawRate;
		float ThrottleScaled;

		GetPilotDesiredAngleRates(PilotInput.X, PilotInput.Y, PilotInput.Z, TargetRollRate, TargetPitchRate, TargetYawRate);
		//TargetYawRate = GetPilotDesiredYawRate(PilotInput.Z);
		ThrottleScaled = GetPilotDesiredThrottle(PilotInput.W);

		InputRateBodyRollPitchYaw(TargetRollRate, TargetPitchRate, TargetYawRate);
		EngineController->SetDesiredThrottlePercent(ThrottleScaled);
	}


	/*--- CALCULATE PILOTs DESIRE ---*/

	// GetPilotDesiredLeanAngles - transform pilot's roll or pitch input into a desired lean angle 
	// returns desired angles in degrees 
	void GetPilotDesiredLeanAngles(float RollIn, float PitchIn, float &RollOut, float &PitchOut)
	{
		// Limit AngleMax 
		AngleMax = FMath::Clamp<float>(AngleMax, 0.0f, 80.0f);

		// Circular limit Roll and Pitch Inputs 
		FVector2D CircularIn = FVector2D(RollIn, PitchIn);
		float TotalIn = CircularIn.Size();
		if (TotalIn > 1.0f)
		{
			float ratio = 1.0f / TotalIn;
			RollIn *= ratio;
			PitchIn *= ratio;
		}

		// scale RollIn, PitchIn to AngleMax range 
		RollIn *= AngleMax;
		PitchIn *= AngleMax;

		// return 
		RollOut = RollIn;
		PitchOut = PitchIn;
	}


	// GetPilotDesiredAngleRates - transform pilot's roll pitch and yaw input into a desired lean angle rates 
	// returns desired angle rates in degrees-per-second 
	void GetPilotDesiredAngleRates(float RollIn, float PitchIn, float YawIn, float &RollRateOut, float &PitchRateOut, float &YawRateOut)
	{
		//float RateLimit;
		float RollOut;
		float PitchOut;
		float YawOut;

		// Circular limit Roll and Pitch Inputs 
		FVector2D CircularIn = FVector2D(RollIn, PitchIn);
		float TotalIn = CircularIn.Size();
		if (TotalIn > 1.0f)
		{
			float ratio = 1.0f / TotalIn;
			RollIn *= ratio;
			PitchIn *= ratio;
		}

		// expo variables 
		float RPIn3, RPOut;

		// range check expo 
		AccroRollPitchExpo = FMath::Clamp(AccroRollPitchExpo, -0.5f, 1.0f);

		// roll expo 
		RPIn3 = RollIn * RollIn * RollIn;
		RPOut = (AccroRollPitchExpo * RPIn3) + ((1.0f - AccroRollPitchExpo) * RollIn);
		RollOut = RPOut * AccroRollPitchPGain;

		// pich expo 
		RPIn3 = PitchIn * PitchIn * PitchIn;
		RPOut = (AccroRollPitchExpo * RPIn3) + ((1.0f - AccroRollPitchExpo) * PitchIn);
		PitchOut = RPOut * AccroRollPitchPGain;

		// calculate yaw rate request 
		YawOut = GetPilotDesiredYawRate(YawIn);

		RollRateOut = RollOut;
		PitchRateOut = PitchOut;
		YawRateOut = YawOut;

	}


	// GetPilotDesiredYawRate - transform pilot's yaw input into a desired yaw rate 
	// returns desired yaw rate in degrees per second 
	float GetPilotDesiredYawRate(float YawIn)
	{
		float YawRequest;

		// expo variables 
		float YawIn3, YawOut;

		// range check expo 
		AccroYawExpo = FMath::Clamp(AccroYawExpo, -0.5f, 1.0f);

		// yaw expo 
		YawIn3 = YawIn * YawIn * YawIn;
		YawOut = (AccroYawExpo * YawIn3) + ((1.0f - AccroYawExpo) * YawIn);
		YawRequest = YawOut * YawPGain;
	
		// convert pilot input to the desired yaw rate 
		return YawRequest;
	}


	// GetPilotDesiredThrottle transform pilot's manual throttle input to make hover throttle mid stick 
	// used only for manual throttle modes 
	// returns throttle output 0 to 1 
	float GetPilotDesiredThrottle(float ThrottleIn)
	{
		
		float MidStick = InputController->GetThrottleMidStick();
		float ThrottleMidIn = EngineController->GetThrottleHover();

		// ensure reasonable throttle values 
		ThrottleIn = FMath::Clamp<float>(ThrottleIn, 0.0f, 1.0f);

		// calculate normalised throttle input 0..1 and mid = 0.5
		if (ThrottleIn < MidStick) {
			// below the deadband 
			ThrottleIn = ThrottleIn * 0.5f / MidStick;
		}
		else if (ThrottleIn > MidStick) {
			// above the deadband 
			ThrottleIn = 0.5f + (ThrottleIn - MidStick) * 0.5f / (1.0f - MidStick);
		}
		else {
			// must be in the deadband 
			ThrottleIn = 0.5f;
		}

		// Expo 
		//float Expo = FMath::Clamp<float>(-(ThrottleMidIn - 0.5) / 0.375, -0.5f, 1.0f); // calculate the output throttle using the given expo function 
		float Expo = -(ThrottleMidIn - 0.5) / 0.375; 
		
		float ThrottleOut = (ThrottleIn * (1 - Expo) + Expo * ThrottleIn*ThrottleIn*ThrottleIn);

		//!!! We have a  problem with the Expo function. If we clamp it, we dont hover, if we dont: ThrottleOut(1) != 1
		//UE_LOG(LogTemp, Display, TEXT("%f %f %f %f"), Expo, ThrottleIn, ThrottleMidIn, ThrottleOut);
		
		return ThrottleOut;
	}



	// GetPilotDesiredClimbRate - transform pilot's throttle input to climb rate in m/s 
	// without any deadzone at the bottom 
	float GetPilotDesiredClimbRate(float ThrottleIn)
	{
		float DesiredRate;

		float MidStick = InputController->GetThrottleMidStick();

		// ensure a reasonable deadzone 
		ThrottleDeadzone = FMath::Clamp<float>(ThrottleDeadzone, 0.0f, 0.4f);

		float DeadbandTop = MidStick + ThrottleDeadzone;
		float DeadbandBottom = MidStick - ThrottleDeadzone;

		// ensure a reasonable throttle value 
		ThrottleIn = FMath::Clamp<float>(ThrottleIn, 0.0f, 1.0f);

		// check throttle is above, below or in the deadband 
		if (ThrottleIn < DeadbandBottom) {
			// below the deadband 
			DesiredRate = PilotSpeedDown * (ThrottleIn - DeadbandBottom) / DeadbandBottom;
		}
		else if (ThrottleIn > DeadbandTop) {
			// above the deadband 
			DesiredRate = PilotSpeedUp * (ThrottleIn - DeadbandTop) / (1.0f - DeadbandTop);
		}
		else {
			// must be in the deadband 
			DesiredRate = 0.0f;
		}

		return DesiredRate;
	}



	/*--- INPUT FUNCTIONS: INPUT DATA INTO FLIGHT CONTROLLER ---*/

	// Command an angular roll, pitch and rate yaw with angular velocity feedforward 
	void InputAngleRollPitchRateYaw(float RollIn, float PitchIn, float YawRateIn)
	{
		//
		// Rotate Target around Yaw Rate Quat
		//
		
		//Yaw Rate World Rotation increment
		FRotator YawRateRotator = FRotator(0.0, YawRateIn * DeltaTime, 0.0f);
		YawRateRotator = YawRateRotator.Clamp();
		FQuat YawRateUpdateQuat = FQuat(YawRateRotator);
		// Rotate for Yaw around World (z) axis
		AttitudeTargetQuat = YawRateUpdateQuat * AttitudeTargetQuat;
		// .. and update Attitude Target
		AttitudeTargetQuat.Normalize();

		//
		// Rotate towards RollIn, PitchIn with <= MaxSpeed
		//

		//  Calculate desired Attitude from Roll and pitch angles
		FRotator AttitudeTargetRotator = AttitudeTargetQuat.Rotator();
		AttitudeTargetRotator.Roll = -RollIn;
		AttitudeTargetRotator.Pitch = -PitchIn;
		AttitudeTargetRotator.Yaw = AttitudeTargetQuat.Rotator().Yaw ;
		AttitudeTargetRotator = AttitudeTargetRotator.Clamp();
		// Compute quaternion target attitude
		FQuat AttitudeDesiredTargetQuat = FQuat(AttitudeTargetRotator);
		AttitudeDesiredTargetQuat.Normalize();

		// Speed limit the rotation from Targe Attitude to new desired target Attitude

		// 1.) Get shortest arc Delta Rot and Limit Rota Speed to AccroRollPitchPGain
		float Direction = ((AttitudeDesiredTargetQuat | AttitudeTargetQuat) >= 0) ? 1.0f : -1.0f;
		FQuat DeltaQuat  = (AttitudeDesiredTargetQuat * Direction) * AttitudeTargetQuat.Inverse(); 
		DeltaQuat.Normalize();

		// 2.) Calc  Desired Velocity for Delta Rot from 1)
		
		//convert to angle axis representation so we can do math with angular velocity 
		FVector Axis = FVector::ZeroVector;
		float Angle = 0.0f;
		DeltaQuat.ToAxisAndAngle(Axis, Angle); 
		Axis.Normalize();

		// We need the max RP turn rates in rads for clamping angular velocities
		float MaxRPVelocityRads = FMath::DegreesToRadians(AccroRollPitchPGain * DeltaTime);

		// We speed limit the velocity
		float ClampedAngle = FMath::Clamp(Angle, 0.0f, MaxRPVelocityRads);
		// Recreate DeltaQuat, now with limited speed
		DeltaQuat = FQuat(Axis, ClampedAngle);
		DeltaQuat.Normalize();

		// Calculate new TargetQuat
		AttitudeTargetQuat = DeltaQuat * AttitudeTargetQuat;
		AttitudeTargetQuat.Normalize();

		//
		// Perform Calculated Rotation from AttitudeQuat to AttitudeTargetQuat
		//

		RunQuat();
	}
	

	void InputRateBodyRollPitchYaw(float RollRateIn, float PitchRateIn, float YawRateIn)
	{
		FRotator RateRotator = FRotator(-PitchRateIn * DeltaTime, YawRateIn * DeltaTime, -RollRateIn*DeltaTime);
		RateRotator = RateRotator.Clamp();
		FQuat AttitudeTargetUpdateQuat = FQuat(RateRotator);

		AttitudeTargetQuat = AttitudeTargetQuat * AttitudeTargetUpdateQuat;
		AttitudeTargetQuat.Normalize();

		// Call quaternion attitude controller
		RunQuat();
	}

	/* --- RUN QUAT --- */

	void RunQuat()
	{

		// We need the max turn rates in rads for clamping angular velocities
		float MaxRPVelocityRad = FMath::DegreesToRadians(AccroRollPitchPGain);
		float MaxYVelocityRad = FMath::DegreesToRadians(YawPGain);

		// Get vehicles current orientation
		FTransform bodyTransform =  BodyInstance->GetUnrealWorldTransform();
		FQuat AttitudeVehicleQuat = bodyTransform.GetRotation();
		// FQuat AttitudeTargetQuat ist the desired rotation

		//q will rotate from our current rotation to desired rotation 
		float Direction = ((AttitudeTargetQuat | AttitudeVehicleQuat) >= 0) ? 1.0f : -1.0f;
		FQuat DeltaQuat  = (AttitudeTargetQuat * Direction) * AttitudeVehicleQuat.Inverse(); 
		DeltaQuat.Normalize();
		
		//convert to angle axis representation so we can do math with angular velocity 
		FVector Axis = FVector::ZeroVector;
		float Angle = 0.0f;
		DeltaQuat.ToAxisAndAngle(Axis, Angle); 
		Axis.Normalize();
		
		// AngularVelocityTgt is the w we need to achieve in Rads
		FVector AngularVelocityTgt = Axis * Angle / DeltaTime; 

		// Get current angular Velocity in World Space in Rads
        FVector AngularVelocityNow = BodyInstance->GetUnrealWorldAngularVelocityInRadians();

		// AngularVelocityToApply is the w we need to Apply to physx directly or after torque calculation
		FVector AngularVelocityToApply = FVector::ZeroVector;
		
		// Make all Velocity Vectors local space
		AngularVelocityTgt = bodyTransform.InverseTransformVectorNoScale(AngularVelocityTgt);
		AngularVelocityToApply = bodyTransform.InverseTransformVectorNoScale(AngularVelocityToApply);
		AngularVelocityNow = bodyTransform.InverseTransformVectorNoScale(AngularVelocityNow);
		
		// AngularVelocityToApply depends on the choosen ControlLoop
		if(RotationControlLoop == EControlLoop::ControlLoop_P)
		{
			// For Option a) Calculate the (raw) Velocity we have to apply by taking into account, that we have Velocity allready
			AngularVelocityToApply = AngularVelocityTgt - AngularVelocityNow;
		}
		else if (RotationControlLoop == EControlLoop::ControlLoop_PID)
		{
			// For Option b) Run the PID-Controllers to find PID Angular Velocity to Apply in rads 
			AngularVelocityToApply = FVector (
				StepRateRollPid(AngularVelocityNow.X, AngularVelocityTgt.X),
				StepRatePitchPid(AngularVelocityNow.Y, AngularVelocityTgt.Y),
				StepRateYawPid(AngularVelocityNow.Z, AngularVelocityTgt.Z)
        	);
		}
		else if (RotationControlLoop == EControlLoop::ControlLoop_SPD)
		{
			// For Option c) Run the FPD-Controllers to find SPD Angular Velocity to Apply in rads 
    	    AngularVelocityToApply = StepRateRollSpd( AngularVelocityNow, AngularVelocityTgt);
		}


		// We clamp Angular Velocity to the requested max
		AngularVelocityToApply.X = FMath::Clamp(AngularVelocityToApply.X, -MaxRPVelocityRad, MaxRPVelocityRad);
		AngularVelocityToApply.Y = FMath::Clamp(AngularVelocityToApply.Y, -MaxRPVelocityRad, MaxRPVelocityRad);
		AngularVelocityToApply.Z = FMath::Clamp(AngularVelocityToApply.Z, -MaxYVelocityRad, MaxYVelocityRad);

///*
		// OPTION #0: This is the desired Option. The others are for debugging purposes only. 
		// Send Calculated Roll Acceleration in rads to the Engine Controller
		FVector DesiredEngineRotation = FVector(
			AngularVelocityToApply.X / MaxRPVelocityRad,
			AngularVelocityToApply.Y / MaxRPVelocityRad,
			AngularVelocityToApply.Z / MaxYVelocityRad
		);
		DesiredEngineRotation /= DeltaTime;
		EngineController->SetDesiredRotationForces(DesiredEngineRotation); // local space !!
//*/

/// For following Options it is better to disable gravity of the root mesh for debug purposes

/*
		// OPTION #1: Rotate direct. Not recommended
		FHitResult Hit = FHitResult();
		PrimitiveComponent->AddWorldRotation(DeltaQuat, false, &Hit, ETeleportType::TeleportPhysics);
*/

/*
		// OPTION #2: Set Velocity in Physx directy (not recommended). Use only withot StabilizerLoop (RotationControlLoop = EControlLoop::ControlLoop_None)
		FVector AngularVelocityGlobal = bodyTransform.TransformVectorNoScale(AngularVelocityToApply);
		PrimitiveComponent->SetPhysicsAngularVelocityInRadians(AngularVelocityGlobal, true, NAME_None); // global space
*/

/*
		// OPTION #3: Simulate Velocity-Change in rads by Impulse, dont care about Inertia, mass etc.
		FVector AngularImpulseGlobal = bodyTransform.TransformVectorNoScale(AngularVelocityToApply);
		BodyInstance->AddAngularImpulseInRadians(AngularImpulseGlobal, true); // global space
*/

/*
		// OPTION #4: Simulate Acceleration-Change in rads by Torque, dont care about Inertia, mass etc.
		FVector AngularAccelerationGlobal = bodyTransform.TransformVectorNoScale(AngularVelocityToApply);
		BodyInstance->AddTorqueInRadians(AngularAccelerationGlobal / DeltaTime, false, true); // global space
*/
/*
		// OPTION #5: Simulate Velocity-Change in rads by Impulse, use Inertia Tensor 
		FVector AngularVelocityLocal = AngularVelocityToApply;
		AngularVelocityLocal *= BodyInstance->GetBodyInertiaTensor(); 
		FVector TorqueWorld = bodyTransform.TransformVectorNoScale(AngularVelocityLocal); 
		BodyInstance->AddAngularImpulseInRadians(TorqueWorld, false); // global space
*/	
/*
		// OPTION #6: Simulate Acceleration-Change in rads by Torque, use Inertia Tensor 
		FVector AngularAccelerationLocal = AngularVelocityToApply / DeltaTime;
		AngularAccelerationLocal *= BodyInstance->GetBodyInertiaTensor(); 
		FVector TorqueWorld = bodyTransform.TransformVectorNoScale(AngularAccelerationLocal); 
		BodyInstance->AddTorqueInRadians(TorqueWorld, false, false);  // global space
*/	

	}

	// Run the roll angular velocity PID controller and return the output in rads
	float StepRateRollPid(float RateActualRads, float RateTargetRads) 
	{ 
		float RateActualNorm = RateActualRads / FMath::DegreesToRadians(AccroRollPitchPGain);
		float RateTargetNorm = RateTargetRads / FMath::DegreesToRadians(AccroRollPitchPGain);
		return RateRollPid.Calculate(RateTargetNorm, RateActualNorm, DeltaTime) * FMath::DegreesToRadians(AccroRollPitchPGain);
	} 

	// Run the pitch angular velocity PID controller and return the output in rads
	float StepRatePitchPid(float RateActualRads, float RateTargetRads) 
	{ 
		float RateActualNorm = RateActualRads / FMath::DegreesToRadians(AccroRollPitchPGain);
		float RateTargetNorm = RateTargetRads / FMath::DegreesToRadians(AccroRollPitchPGain);
		return RatePitchPid.Calculate(RateTargetNorm, RateActualNorm, DeltaTime) * FMath::DegreesToRadians(AccroRollPitchPGain);
	} 

	// Run the roll angular velocity PID controller and return the output in rads
	float StepRateYawPid(float RateActualRads, float RateTargetRads) 
	{ 
		float RateActualNorm = RateActualRads / FMath::DegreesToRadians(YawPGain);
		float RateTargetNorm = RateTargetRads / FMath::DegreesToRadians(YawPGain);
		return RateYawPid.Calculate(RateTargetNorm, RateActualNorm, DeltaTime) * FMath::DegreesToRadians(YawPGain);
	} 

	void Debug(FColor ColorIn, FVector2D DebugFontSizeIn)
	{
	//	GEngine->AddOnScreenDebugMessage(-1, 0, ColorIn, FString::Printf(TEXT("Engines %%  : 1=%f 2=%f 3=%f 4=%f"), GetEnginePercent(0), GetEnginePercent(1), GetEnginePercent(2), GetEnginePercent(3)), true, DebugFontSizeIn);
		
	}


// Run the rotational angular velocity FPD controller and return the output detla w in rads
	FVector StepRateRollSpd(FVector Current, FVector Target)
	{
		float kp = SPDFrequency * SPDFrequency * 9.0f; 
		float kd = 4.5f * SPDFrequency * SPDDamping; 
		float dt = DeltaTime; 
		
		float g = 1.0f / (1.0f + kd * dt + kp * dt * dt); 
		float kpg = kp * g; 
		float kdg = (kd + kp * dt) * g; 

		return FVector(kpg * Target - kdg * Current);
	}


};


