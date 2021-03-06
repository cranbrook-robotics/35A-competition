#pragma config(UserModel, "C:/Users/rstudent/code/robot-configs/35A-in-and-out.c")
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX)

//Competition Control and Duration Settings
#pragma competitionControl(Competition)
#pragma autonomousDuration(20)
#pragma userControlDuration(120)

#include "Vex_Competition_Includes.c"   //Main competition background code...do not modify!

#define POWER_EXPANDER_A1
#include <CKFlywheelSpeedController.h>


const float DriveWheelDiameter = 4.15;//inches
const float DriveWheelCircumfrence = DriveWheelDiameter * PI;//inches
const float WheelbaseWidth = 14.5;//inches
const float WheelbaseLength = 11.5;//inches (1/2 inch per vex hole)
const float WheelbaseDiagonal = sqrt( WheelbaseWidth*WheelbaseWidth + WheelbaseLength*WheelbaseLength );

// If the arc length of a spin turn is X, then the omni wheels need to travel X*[this factor]
// (derived triginometrically)
const float DrivetrainSpinDisplacementFactor = WheelbaseWidth / WheelbaseDiagonal;




int feetToTicks( float feet ){
	static const float conversion = 12.0 / DriveWheelCircumfrence * TicksPerRev_393Turbo;
	return (int)( feet * conversion );
}

float feetToDegrees( float feet ){
	static const float conversion = 12.0 / DriveWheelCircumfrence * 360;
	return (int)( feet * conversion );
}


float turningDistanceFeet( float degrees ){
	float radians = degreesToRadians(degrees);
	float arcLength = radians * (WheelbaseDiagonal / 2) / 12;
	return arcLength * DrivetrainSpinDisplacementFactor;
}

FlywheelSpeedController speedCtlr;


task FlywheelSpeedControl() {
	while(true){
		update(speedCtlr);
		delay(50);
	}
}


void setDrive( int left, int right )
{
	motor[mDriveL] = left;
	motor[mDriveR] = right;
}

void setIntakeRoller( int power ){
	motor[mIntakeRoller] = power;
}

void setIntakeChain( int power ){
	motor[mIntakeF] = motor[mIntakeM] = motor[mIntakeB] = power;
}


int getIMETicks(tMotor port){
	int ticks = nMotorEncoder[port];
	nMotorEncoder[port] = 0;
	return ticks;
}

//float driveDistanceFeet = 0;



float slowingFactor( int remainingTicks, int slowingDistanceTicks ){
	if( remainingTicks > slowingDistanceTicks )
		return 1;
	return pow( 2, (float)remainingTicks / slowingDistanceTicks ) - 1;
}


//task driveStraightTask(){
void go( float driveDistanceFeet, int dL = 1, int dR = 1 ){
	int ticks = feetToTicks( driveDistanceFeet );
	int slowingDistanceTicks = feetToTicks( minimum(driveDistanceFeet / 2, 2) );
	//int ticksL = ticks * dL;
	//int ticksR = ticks * dR;
	int basePower = 50;
	getIMETicks(mDriveL);
	getIMETicks(mDriveR);
	setDrive( dL*basePower, dR*basePower );
	delay(100);
	int leftTicks = 0, rightTicks = 0;
	int remainingL = 1;
	int remainingR = 1;
	while( remainingL > 0 || remainingR > 0 ){
		leftTicks += getIMETicks(mDriveL);
		rightTicks += getIMETicks(mDriveR);
		remainingL = ticks - dL*leftTicks;
		remainingR = ticks - dR*rightTicks;
		int error = dL*leftTicks - dR*rightTicks;
		int turningOffset = (int)(1.5 * error);
		//int _basePowerL = (int)( slowingFactor(remainingL, slowingDistanceTicks) * basePower );
		//int _basePowerR = (int)( slowingFactor(remainingR, slowingDistanceTicks) * basePower );
		int _basePowerL = basePower, _basePowerR = basePower;
		setDrive( dL*_basePowerL - turningOffset, dR*_basePowerR + turningOffset );
		delay(20);
	}
	setDrive(0,0);
}

//void driveStraight( float distFt ){
//	driveDistanceFeet = distFt;
//	startTask( driveStraightTask );

//	//float angle = feetToDegrees(driveDistanceFeet);
//	//setMotorTarget(mDriveL, angle, 127);
//	//setMotorTarget(mDriveR, angle, 127);
//}



/////////////////////////////////////////////////////////////////////////////////////////
//
//                          Pre-Autonomous Functions
//
// You may want to perform some actions before the competition starts. Do them in the
// following function.
//
/////////////////////////////////////////////////////////////////////////////////////////

void pre_auton()
{
  // Set bStopTasksBetweenModes to false if you want to keep user created tasks running between
  // Autonomous and Tele-Op modes. You will need to manage all user created tasks if set to false.
  bStopTasksBetweenModes = true;


	// power = A e^( B speed )
	//const float A = 1.2235, B = 0.1072;
  const float A = 0.7686, B = 0.1304; // April 13th recharacterization

	// Controller coefficients
	const float Kq = 0.2, Ki = 0.02, Kd = 0;

	const tMotor motorPorts[] =	{ mFly1, mFly2, mFly3, mFly4 };

  FlywheelSpeedControllerInit( speedCtlr, Kq, Ki, Kd, A, B, motorPorts, 4, M393HighSpeed );

  // half of the flywheel motors are on the main brain battery; other half on the power expander.
  setFlywheelBatteryConfig( speedCtlr, vPowerExpander, 0.5 );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 Autonomous Task
//
// This task is used to control your robot during the autonomous phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

task autonomous()
{
	startTask( FlywheelSpeedControl, kHighPriority );

	//go( 4 );
	//delay(4000);
	go( turningDistanceFeet(90), 1, -1 );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 User Control Task
//
// This task is used to control your robot during the user control phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

task usercontrol()
{
	const float FlyspeedMin = 7.5, FlyspeedMid = 10, FlyspeedMax = 16, FlyspeedIncrement = 0.5;

	float flyspeed = FlyspeedMin;
	bool isFlywheelOn = false;

	startTask( FlywheelSpeedControl, kHighPriority );

	time1[T1] = 0;
	while (true)
	{
		setDrive( vexRT[ChJoyLY], vexRT[ChJoyRY] );
		setIntakeRoller( buttonsToPower(Btn5D, Btn5U) );
		setIntakeChain( buttonsToPower(Btn6D, Btn6U) );

		int turnOnFlywheel = vexRT[Btn8L];
		int turnOffFlywheel = vexRT[Btn8R];

		if( turnOnFlywheel || turnOffFlywheel ){
			isFlywheelOn = (bool)turnOnFlywheel;
		}

		int speedUpFlywheel = vexRT[Btn8U];
		int slowDownFlywheel = vexRT[Btn8D];
		if( speedUpFlywheel || slowDownFlywheel ){
			flyspeed += (speedUpFlywheel ? +FlyspeedIncrement : -FlyspeedIncrement);
			flyspeed = bound( flyspeed, FlyspeedMin, FlyspeedMax );
			delay(100);
		}

		if( vexRT[Btn7L] ){
			flyspeed = FlyspeedMax;
			isFlywheelOn = true;
		}

		if( vexRT[Btn7R] ){
			flyspeed = FlyspeedMin;
			isFlywheelOn = true;
		}

		if( vexRT[Btn7U] ){
			flyspeed = FlyspeedMid;
			isFlywheelOn = true;
		}

		setTargetSpeed( speedCtlr, isFlywheelOn ? flyspeed : 0 );

		delay(10);
	}
}
//}
//if ( limitswitch = true){
//	limitcount = 1;

//	if (limitcount = 1 && limitswitch = false) {
//		IntakeDown until limitswitch = true
//	}
