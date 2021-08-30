FROM node:16

# Download and extract Android NDK for ndk-stack
RUN curl -Lo /tmp/ndk.zip "https://dl.google.com/android/repository/android-ndk-r23-linux.zip" \
  && unzip /tmp/ndk.zip -d /usr/src/android-ndk \
  && rm /tmp/ndk.zip

# Copy debug libs
ADD debug-libs /usr/src/debug-libs

# Setup Node.js server
WORKDIR /usr/src/app
ADD server ./
RUN npm install
EXPOSE 3000
CMD npm start
