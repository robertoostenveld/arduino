/*
 * This is a reorganized version of https://github.com/kriswiner/MPU9250
 */

#ifndef __AHRS_H_
#define __AHRS_H_

class ahrs {
  public:
    ahrs(void);
    ~ahrs(void);
    void begin(void);
    void update(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float);
    float pitch, yaw, roll;

  private:
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);

    // There is a tradeoff in the beta parameter between accuracy and response speed.
    // In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
    // However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
    // Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
    // By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
    // I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
    // the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
    // In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
    float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta
    float zeta = sqrt(3.0f / 4.0f) * GyroMeasDrift;   // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value

    // these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback, Ki for integral
#define Kp 2.0f * 5.0f
#define Ki 0.0f

    float a12, a22, a31, a32, a33;            // rotation matrix coefficients for Euler angles and gravity components

    float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
    uint32_t lastUpdate = 0, firstUpdate = 0; // used to calculate integration interval
    uint32_t Now = 0;                         // used to calculate integration interval

    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
    float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method

    float lin_ax, lin_ay, lin_az;             // linear acceleration (acceleration with gravity component subtracted)

    // global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
    float GyroMeasError = PI * (40.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
    float GyroMeasDrift = PI * (0.0f  / 180.0f);   // gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)

}; // class

#endif
