FROM node:16

# Download and extract Android NDK for ndk-stack
RUN curl -Lo /tmp/ndk.zip "https://dl.google.com/android/repository/android-ndk-r23-linux.zip" \
  && unzip /tmp/ndk.zip -d /usr/src \
  && mv /usr/src/android-ndk-r23 /usr/src/android-ndk \
  && rm /tmp/ndk.zip

# Copy debug libs
ADD debug-libs /usr/src/debug-libs

# Server setup
WORKDIR /usr/src/app

# Cache npm packages in a separate layer
ADD server/package*.json ./
RUN npm install

ADD server ./
CMD npm start
