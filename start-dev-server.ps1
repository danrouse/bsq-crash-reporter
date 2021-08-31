$dockerArgs = $args
if (!$dockerArgs) {
  $dockerArgs = @("npm", "run", "start-dev")
}

docker run -p 3000:3000 `
  --mount src=$pwd/server,target=/usr/src/app,type=bind `
  --mount src=$pwd/debug-libs,target=/usr/src/debug-libs,type=bind `
  -ti bsq-crash-reporter $dockerArgs
