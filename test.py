import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Input, LSTM, Dropout, Dense

n_timesteps = 20
n_features = 3

model = Sequential()
model.add(LSTM(64, input_shape=(n_timesteps,n_features)))
model.add(Dropout(0.5))
model.add(Dense(64, activation='relu'))
model.add(Dense(10, activation='softmax'))
model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

model.summary()

from keras2cpp.keras2cpp import export_model
export_model(model, './model/example.model')

