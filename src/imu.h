#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

static const int zwift[361] =  {
0,
1,
2,
3,
4,
5,
6,
7,
8,
9,
10,
11,
12,
13,
14,
15,
16,
17,
18,
19,
20,
21,
22,
23,
24,
25,
26,
27,
28,
29,
30,
31,
32,
33,
34,
35,
36,
37,
38,
39,
40,
41,
42,
43,
44,
45,
46,
47,
48,
49,
50,
51,
52,
53,
54,
55,
56,
57,
58,
59,
60,
61,
62,
63,
64,
65,
66,
67,
68,
69,
70,
71,
72,
73,
74,
75,
76,
77,
78,
79,
80,
81,
82,
83,
84,
85,
86,
87,
88,
89,
90,
91,
92,
93,
94,
95,
96,
97,
98,
99,
100,
101,
102,
103,
104,
105,
106,
107,
108,
109,
110,
111,
112,
113,
114,
115,
116,
117,
118,
119,
120,
121,
122,
123,
124,
125,
126,
127,
128,
129,
130,
131,
132,
133,
134,
135,
136,
137,
138,
139,
140,
141,
142,
143,
144,
145,
146,
147,
148,
149,
150,
151,
152,
153,
154,
155,
156,
157,
158,
159,
160,
161,
162,
163,
164,
165,
166,
167,
168,
169,
170,
171,
172,
173,
174,
175,
176,
177,
178,
179,
180,
-179,
-178,
-177,
-176,
-175,
-174,
-173,
-172,
-171,
-170,
-169,
-168,
-167,
-166,
-165,
-164,
-163,
-162,
-161,
-160,
-159,
-158,
-157,
-156,
-155,
-154,
-153,
-152,
-151,
-150,
-149,
-148,
-147,
-146,
-145,
-144,
-143,
-142,
-141,
-140,
-139,
-138,
-137,
-136,
-135,
-134,
-133,
-132,
-131,
-130,
-129,
-128,
-127,
-126,
-125,
-124,
-123,
-122,
-121,
-120,
-119,
-118,
-117,
-116,
-115,
-114,
-113,
-112,
-111,
-110,
-109,
-108,
-107,
-106,
-105,
-104,
-103,
-102,
-101,
-100,
-99,
-98,
-97,
-96,
-95,
-94,
-93,
-92,
-91,
-90,
-89,
-88,
-87,
-86,
-85,
-84,
-83,
-82,
-81,
-80,
-79,
-78,
-77,
-76,
-75,
-74,
-73,
-72,
-71,
-70,
-69,
-68,
-67,
-66,
-65,
-64,
-63,
-62,
-61,
-60,
-59,
-58,
-57,
-56,
-55,
-54,
-53,
-52,
-51,
-50,
-49,
-48,
-47,
-46,
-45,
-44,
-43,
-42,
-41,
-40,
-39,
-38,
-37,
-36,
-35,
-34,
-33,
-32,
-31,
-30,
-29,
-28,
-27,
-26,
-25,
-24,
-23,
-22,
-21,
-20,
-19,
-18,
-17,
-16,
-15,
-14,
-13,
-12,
-11,
-10,
-9,
-8,
-7,
-6,
-5,
-4,
-3,
-2,
-1,
0
};

TwoWire mybus = TwoWire(0);

// Check I2C device address and correct line below (by default address is 0x29 or 0x28)
//                                   id, address
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28,&mybus);

int init_heading=0;


float getHeading(){
  sensors_event_t event;
  bno.getEvent(&event);
   return zwift[(int)event.orientation.x];
}


void calibrate(int seconds)
{
  int startTime = millis();
  int sampleCount = 0;

  float y_head = 0;
  //Collect Data
  while (millis() < startTime + (1000 * seconds)){
      y_head += getHeading();
      sampleCount++;
      // Serial.printf("%.2f,%.2f,%.2f o/s \r\n", gx, gy, gz);
      delay(10);
  }
  // Serial.print(sampleCount);
  // Serial.println(" Samples Taken");
  // Serial.printf("%.2f,%.2f,%.2f o/s \r\n", gyx, gyy, gyz);
  init_heading = (int)y_head / sampleCount;
  // Serial.printf("%.2f,%.2f,%.2f o/s \r\n", _cx, _cy, _cz);
}

void displaySensorDetails(void)
{
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void imu_setup(void)
{

  /* Initialise the sensor */
  mybus.begin(16, 17, 100000);
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
   
  delay(1000);

  /* Use external crystal for better accuracy */
  bno.setExtCrystalUse(true);

//   displaySensorDetails();
  Serial.print("StartCalibration");
  calibrate(1);
  Serial.print("Finished Calibaration::");
  Serial.print(init_heading);
  Serial.println("");
  delay(10);
}

int getCurrentAngle(){
    int current_angle=(int)getHeading()-init_heading;
    return _max(_min(current_angle,45),-45);
}