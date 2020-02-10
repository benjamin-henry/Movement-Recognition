#include <iostream>
#include "keras_model.h"
#include "utils.h"
#include "config.h"



Tensor out;
KerasModel model;
int argMax = -1;
float maxout = -1;

void setup() {
  Serial.begin(115200);
  model.LoadModel();
  // Create a 1D Tensor of shape (SEGMENT_TIME_SIZE, N_FEATURES) for input data.
  Tensor in(config::SEGMENT_TIME_SIZE, config::N_FEATURES);
  in.data_ = { -14.44,  -1.68,  -2.40,
               -14.44,  -1.68,  -2.40,
               -14.63,   3.36,  -2.35,
               -14.63,   3.36,  -2.35,
               -14.63,   3.36,  -2.35,
               -8.21,   1.66,  -6.38,
               -8.21,   1.66,  -6.38,
               -2.88,  -0.87,  -4.96,
               -2.88,  -0.87,  -4.96,
               -2.88,  -0.87,  -4.96,
               -2.88,  -0.87,  -4.96,
               -1.91,  -4.10,  -3.47,
               -1.91,  -4.10,  -3.47,
               -1.91,  -4.10,  -3.47,
               -7.67,   2.92,  -5.25,
               -7.67,   2.92,  -5.25,
               -7.67,   2.92,  -5.25,
               -20.00,  19.99, -12.99,
               -20.00,  19.99, -12.99,
               -20.00,  19.99, -12.99,
               -20.00,  19.99, -16.21,
               -20.00,  19.99, -16.21,
               -7.29,   0.82,  -4.27,
               -7.29,   0.82,  -4.27,
               -7.29,   0.82,  -4.27,
               -4.09,  -1.87,  -4.49,
               -4.09,  -1.87,  -4.49,
               -5.97,  -1.87,  -3.6 ,
               -5.97,  -1.87,  -3.6 ,
               -5.97,  -1.87,  -3.6 ,
               -0.83,  -5.19,  -3.81,
               -0.83,  -5.19,  -3.81,
               -0.83,  -5.19,  -3.81,
               -20.00,  18.28, -10.44,
               -20.00,  18.28, -10.44,
               -20.00,  18.28, -10.44,
               -20.00,  16.40, -14.51,
               -20.00,  16.40, -14.51,
               -20.00,  16.40, -14.51,
               -3.55,  -2.53,  -3.92
             };


  model.Apply(&in, &out);
  //std::cout << softmax_to_label(out.data_) << std::endl;
}

void loop() {
  Serial.println();
  for (int i = 0; i < 7; i++) {
    Serial.print(out.data_[i]);
    Serial.print('\t');
    if (out.data_[i] > maxout) {
      maxout = out.data_[i];
      argMax = i;
    }
  }
  Serial.println(argMax);
  argMax = -1;
  maxout = -1;
}
