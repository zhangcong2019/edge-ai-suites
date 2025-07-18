# Storing frames to S3 storage

Applications can take advantage of S3 publish feature from DLStreamer Pipeline Server and use it to save frames to an S3 compatible storage.

## Steps

> **Note** For the purpose of this demonstration, we'll be using MinIO as the S3 storage. The necessary compose configuration for MinIO microservice is already part of the docker compose file.

1. Install the pip package boto3 in your python environment once if not installed with the following command
    ```sh
    pip3 install boto3==1.36.17
    ```
    > **Note** DLStreamer Pipeline Server expects the bucket to be already present in the database. The next step will help you create one.
    
2. Create a S3 bucket using the following script.

   ```python
   import boto3
   url = "http://<HOST_IP>:8000"
   user = "<value of MR_MINIO_ACCESS_KEY used in .env>"
   password = "<value of MR_MINIO_SECRET_KEY used in .env>"
   bucket_name = "ecgdemo"

   client= boto3.client(
               "s3",
               endpoint_url=url,
               aws_access_key_id=user,
               aws_secret_access_key=password
   )
   client.create_bucket(Bucket=bucket_name)
   buckets = client.list_buckets()
   print("Buckets:", [b["Name"] for b in buckets.get("Buckets", [])])
   ```

3. Start the pipeline with the following cURL command  with `<HOST_IP>` set to system IP. Ensure to give the correct path to the model as seen below. This example starts an AI pipeline.

    ```sh
    curl http://<HOST_IP>:8080/pipelines/user_defined_pipelines/worker_safety_gear_detection_s3write -X POST -H 'Content-Type: application/json' -d '{
        "source": {
            "uri": "file:///home/pipeline-server/resources/videos/Safety_Full_Hat_and_Vest.mp4",
            "type": "uri"
        },
        "destination": {
            "frame": {
                "type": "webrtc",
                "peer-id": "worker_safety_gear_detection_s3"
            }
        },
        "parameters": {
            "detection-properties": {
                "model": "/home/pipeline-server/resources/models/worker-safety-gear-detection/deployment/detection_1/model/model.xml",
                "device": "CPU"
            }
        }
    }'
    ```

4. Go to MinIO console on `http://<HOST_IP>:8000/` and login with `MR_MINIO_ACCESS_KEY` and `MR_MINIO_SECRET_KEY` provided in `.env` file. After logging into console, you can go to `ecgdemo` bucket and check the frames stored.

   ![S3 minio image storage](./images/s3-minio-storage.png)
