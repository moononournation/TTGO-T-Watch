#define LEFT_A 13
#define LEFT_B 14
#define RIGHT_A 25
#define RIGHT_B 26

#define MOTOR_PWM 255
#define DELAY_TIME 600
#define LOOP_COUNT 5

static bool touched = false;
void IRAM_ATTR isr()
{
  touched = true;
}

void setup()
{
  pinMode(38, INPUT_PULLUP);
  attachInterrupt(38, isr, FALLING);

  ledcAttachPin(LEFT_A, 1);  // assign LEFT_A pin to channel 1
  ledcSetup(1, 12000, 8);    // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(LEFT_B, 2);  // assign LEFT_B pin to channel 2
  ledcSetup(2, 12000, 8);    // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(RIGHT_A, 3); // assign RIGHT_A pin to channel 3
  ledcSetup(3, 12000, 8);    // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(RIGHT_B, 4); // assign RIGHT_B pin to channel 4
  ledcSetup(4, 12000, 8);    // 12 kHz PWM, 8-bit resolution
  motor(0, 0);
}

void loop()
{
  if (touched)
  {
    int i = LOOP_COUNT;
    while (i--) {
      motor(random(-MOTOR_PWM, MOTOR_PWM), random(-MOTOR_PWM, MOTOR_PWM));
      delay(DELAY_TIME);
    }
    motor(0, 0);
    touched = false;
  }
}

void motor(int left, int right)
{
  if (left == 0)
  {
    ledcWrite(1, 0); // 0 - 255
    ledcWrite(2, 0); // 0 - 255
  }
  else if (left > 0)
  {
    ledcWrite(1, left); // 0 - 255
    ledcWrite(2, 0);    // 0 - 255
  }
  else
  {
    ledcWrite(1, 0);     // 0 - 255
    ledcWrite(2, -left); // 0 - 255
  }

  if (right == 0)
  {
    ledcWrite(3, 0); // 0 - 255
    ledcWrite(4, 0); // 0 - 255
  }
  else if (right > 0)
  {
    ledcWrite(3, right); // 0 - 255
    ledcWrite(4, 0);     // 0 - 255
  }
  else
  {
    ledcWrite(3, 0);      // 0 - 255
    ledcWrite(4, -right); // 0 - 255
  }
}
