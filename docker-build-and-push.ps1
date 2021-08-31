$gcrUrl = "gcr.io/bsq-crash-reporter/bsq-crash-reporter"
docker build --tag bsq-crash-reporter:latest --tag $gcrUrl .
docker push $gcrUrl
& gcloud compute instances update-container bsq-crash-reporter --container-image $gcrUrl
