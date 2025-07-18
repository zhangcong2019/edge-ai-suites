/*
 * INTEL CONFIDENTIAL
 * 
 * Copyright (C) 2021-2024 Intel Corporation.
 * 
 * This software and the related documents are Intel copyrighted materials, and your use of
 * them is governed by the express license under which they were provided to you (License).
 * Unless the License provides otherwise, you may not use, modify, copy, publish, distribute,
 * disclose or transmit this software or the related documents without Intel's prior written permission.
 * 
 * This software and the related documents are provided as is, with no express or implied warranties,
 * other than those that are expressly stated in the License.
*/

#include <boost/exception/all.hpp>

#include <inc/buffer/hvaVideoFrameWithROIBuf.hpp>

#include "nodes/CPU-backend/LocalMultiSensorInputNode.hpp"

#include <thread>

namespace hce{

namespace ai{

namespace inference{

LocalMultiSensorInputNode::LocalMultiSensorInputNode(std::size_t totalThreadNum):hva::hvaNode_t(1, 1, totalThreadNum) {

}

/**
* @brief Parse params, called by hva framework right after node instantiate.
* @param config Configure string required by this node.
*/
hva::hvaStatus_t LocalMultiSensorInputNode::configureByString(const std::string& config) {
    if (config.empty()) return hva::hvaFailure;

    if (!m_configParser.parse(config)) {
      HVA_ERROR("Illegal parse string!");
      return hva::hvaFailure;
    }

    std::string dataSource = "vehicle";
    m_configParser.getVal<std::string>("DataSource", dataSource);
    m_dataSource = dataSource;

    // configure media/radar/depth input indices
    m_configParser.getVal<int>("MediaIndex", m_sensorIndices.mediaIndex);
    m_configParser.getVal<int>("RadarIndex", m_sensorIndices.radarIndex);
    if (m_sensorIndices.mediaIndex >= MULTI_SENSOR_INPUT_NUM || m_sensorIndices.radarIndex >= MULTI_SENSOR_INPUT_NUM) {
        HVA_ERROR("Input sensor index must be smaller than %d, receiving mediaIndex: %d!, RadarIndex: %d", 
                    MULTI_SENSOR_INPUT_NUM, m_sensorIndices.mediaIndex, m_sensorIndices.radarIndex);
        return hva::hvaFailure;
    }

    int inputCapacity = 0;
    m_configParser.getVal<int>("InputCapacity", inputCapacity);
    m_inputCapacity = inputCapacity;

    int stride = 1;
    m_configParser.getVal<int>("Stride", stride);
    m_stride = stride;

    float frameRate = 0.0;
    m_configParser.getVal<float>("FrameRate", frameRate);
    m_frameRate = frameRate;

    std::string controlType = "Video";
    m_configParser.getVal<std::string>("ControlType", controlType);
    m_controlType = controlType;

    transitStateTo(hva::hvaState_t::configured);
    return hva::hvaSuccess;
}

std::shared_ptr<hva::hvaNodeWorker_t> LocalMultiSensorInputNode::createNodeWorker() const{
    return std::shared_ptr<hva::hvaNodeWorker_t>(new LocalMultiInputNodeWorker((hva::hvaNode_t*)this, m_sensorIndices, m_inputCapacity, m_stride, m_frameRate, m_controlType));
} 


LocalMultiInputNodeWorker::LocalMultiInputNodeWorker(hva::hvaNode_t* parentNode, 
        const LocalMultiSensorInputNode::InpustSensorIndices_t &sensorIndices, const size_t &inputCapacity, const size_t &stride, const float &frameRate, const std::string &controlType):
          hva::hvaNodeWorker_t(parentNode), m_ctr(0u), m_sensorIndices(sensorIndices), m_inputCapacity(inputCapacity), m_stride(stride), m_frameRate(frameRate), m_controlType(controlType), m_workStreamId(-1) {
            m_controllerMap[0] = std::make_shared<SendController>(inputCapacity, stride, controlType);
}

void LocalMultiInputNodeWorker::process(std::size_t batchIdx){
    // get input blob from port0
    auto vecBlobInput = getParentPtr()->getBatchedInput(batchIdx, std::vector<size_t>{0});

    if (vecBlobInput.size() != 0) {
        hva::hvaBlob_t::Ptr inBlob = vecBlobInput[0];
        HVA_DEBUG("Local MultiSensor Input node %d on frameId %d", batchIdx, inBlob->frameId);
        hva::hvaBuf_t::Ptr buf = inBlob->get(0);
        std::vector<std::string> comingInputs = buf->get<std::vector<std::string>>();

        // for sanity
        int streamId = (int)inBlob->streamId;
        if (comingInputs.size() == 0 || comingInputs.size() % MULTI_SENSOR_INPUT_NUM != 0) {
            HVA_ERROR("Local MultiSensor Input node %d received invalid inputs length: %d on frame %d, should be a multiple of %d", 
                       batchIdx, comingInputs.size(), inBlob->frameId, MULTI_SENSOR_INPUT_NUM);
        
            HceDatabaseMeta meta;
            sendEmptyBlob(inBlob, true, meta);
            return;
        } else if(m_workStreamId >= 0 && streamId != m_workStreamId){
            HVA_ERROR(
              "Input worker should work on streamId: %d, but received "
              "data from invalid streamId: %d!",
              m_workStreamId, streamId);
          // send output
          HceDatabaseMeta meta;
          sendEmptyBlob(inBlob, true, meta);
          return;

        } else{
            m_workStreamId = streamId;
        }

        boost::filesystem::path p(comingInputs[0]);
        std::string folder = p.parent_path().c_str();
        // std::string radarConfigPath = folder.substr(0, folder.rfind("/")) + ".h5.json";
        // if (!boost::filesystem::exists(radarConfigPath)) {
        //     HVA_ERROR("Local MultiSensor Input node receives radarConfigPath: %s not existing!", radarConfigPath.c_str());
            
        //     HceDatabaseMeta meta;
        //     sendEmptyBlob(inBlob, true, meta);
        //     return;
        // }
        // HVA_DEBUG("Local MultiSensor Input node success to generate radarConfigPath: %s", radarConfigPath.c_str());

        // processing multiple coming medias, send to specified port in sequence
        for (int inputIdx = 0; inputIdx < comingInputs.size(); inputIdx += MULTI_SENSOR_INPUT_NUM) {

            // the last request input will be taged as end
            bool isEnd = (inputIdx >= comingInputs.size() - MULTI_SENSOR_INPUT_NUM);

            /**
             * process input content
             */
            MultiDatasetField_t content;
            HceDatabaseMeta meta;
            // meta.radarConfigPath = radarConfigPath;
            hva::hvaROI_t roi;
            unsigned x = 0, y = 0, w = 0, h = 0;
            roi.x = x;
            roi.y = y;
            roi.width = w;
            roi.height = h;

            auto blob = hva::hvaBlob_t::make_blob();
            blob->streamId = batchIdx;
            blob->frameId = fetch_increment();

            for (int idx = 0; idx < MULTI_SENSOR_INPUT_NUM; idx ++) {

                std::string path = comingInputs[inputIdx + idx];
                // read binary data
                std::fstream fs;
                fs.open(path.c_str(), std::fstream::in | std::fstream::binary);
                if(fs)
                {
                    fs.seekg (0, fs.end);
                    size_t buffLen = fs.tellg();
                    fs.seekg (0, fs.beg);
                    
                    // image
                    if (idx == m_sensorIndices.mediaIndex) {

                        char* buff = new char [buffLen];
                        fs.read(buff, buffLen);
                        content.imageContent.assign((char*)buff, (char*)buff + fs.gcount());
                        content.imageSize = fs.gcount();
                        delete[] buff;
                    }
                    // radar
                    else if (idx == m_sensorIndices.radarIndex) {

                        auto samples_to_read = buffLen / sizeof(std::complex<float>);
                        content.radarContent.resize(samples_to_read);
                        fs.read(reinterpret_cast<char*>(content.radarContent.data()), buffLen);
                        content.radarSize = content.radarContent.size();
                        
                    }
                }
                fs.close();
            }

            HVA_DEBUG("media content size is : %d on frameid %d streamid %d",
                    content.imageSize, blob->frameId, blob->streamId);
            HVA_DEBUG("radar content size is : %d on frameid %d streamid %d",
                    content.radarSize, blob->frameId, blob->streamId);

            // make buffer for blob
            auto jpgHvaBuf = hva::hvaVideoFrameWithROIBuf_t::make_buffer<std::string>(content.imageContent, content.imageSize);
            jpgHvaBuf->rois.push_back(std::move(roi));
            auto radarHvaBuf = hva::hvaVideoFrameWithROIBuf_t::make_buffer<radarVec_t>(content.radarContent, content.radarSize);

            // mark buffer as empty if content is null
            if (content.imageSize == 0 || content.radarSize == 0) {
                jpgHvaBuf->drop = true;
                radarHvaBuf->drop = true;
            }

            if (isEnd) {
                // last one in the batch. Mark it
                HVA_DEBUG("Set tag 1 on frame %d", blob->frameId);
                jpgHvaBuf->tagAs(1);
                radarHvaBuf->tagAs(1);
            } 
            else {
                jpgHvaBuf->tagAs(0);
                radarHvaBuf->tagAs(0);
            }

            if (streamId == m_workStreamId && m_controllerMap.find(m_workStreamId) == m_controllerMap.end() && 0 < m_inputCapacity) {
                // key not exists, construct it
                m_controllerMap[m_workStreamId] = std::make_shared<SendController>(m_inputCapacity, m_stride, m_controlType);
            }

            if ((streamId == m_workStreamId) && (0 < m_inputCapacity)) {
                std::unique_lock<std::mutex> lock(m_controllerMap[m_workStreamId]->mtx);
                while (m_controllerMap[m_workStreamId]->count > (m_inputCapacity - 1) * m_stride) {
                    (m_controllerMap[m_workStreamId]->notFull).wait(lock);
                }
                (m_controllerMap[m_workStreamId]->count) += m_stride;
                lock.unlock();
                jpgHvaBuf->setMeta(m_controllerMap[m_workStreamId]);
                radarHvaBuf->setMeta(m_controllerMap[m_workStreamId]);
            }
            
            jpgHvaBuf->setMeta(meta);
            radarHvaBuf->setMeta(meta);
            jpgHvaBuf->frameId = blob->frameId;
            radarHvaBuf->frameId = blob->frameId;
            HVA_DEBUG("Multi input source node sets meta to buffer");

            blob->push(jpgHvaBuf);
            blob->push(radarHvaBuf);
            sendOutput(blob, 0, std::chrono::milliseconds(0));
            // auto now = std::chrono::high_resolution_clock::now();
            // auto epoch = now.time_since_epoch();
            // auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
            HVA_INFO("Multi input source node push the %d th jpeg data to blob", blob->frameId);
            // HVA_INFO("Multi input source node push the %d th jpeg data to blob on time %d", blob->frameId, milliseconds);
            if ((streamId == m_workStreamId) && (0 >= m_inputCapacity || (m_controllerMap[m_workStreamId]->count > (m_inputCapacity - 1) * m_stride)) && (0 != inputIdx) && (0.0 != m_frameRate)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 / m_frameRate)));
            }
        }
    }
    
}

/**
 * @brief Frame index increased for every coming frame, will be called at the process()
 * @param void
 */
unsigned LocalMultiInputNodeWorker::fetch_increment() { return m_ctr++; }

/**
 * @brief Send empty buf to next node
 * @param blob input blob data with empty buf
 * @param isEnd flag for coming request
 * @param meta databaseMeta
 */
void LocalMultiInputNodeWorker::sendEmptyBlob(hva::hvaBlob_t::Ptr blob,
                                            bool isEnd,
                                            const HceDatabaseMeta& meta) {

    blob->vBuf.clear();
    auto jpgHvaBuf = hva::hvaVideoFrameWithROIBuf_t::make_buffer<std::string>(std::string(), 0);
    auto radarHvaBuf = hva::hvaVideoFrameWithROIBuf_t::make_buffer<radarVec_t>(radarVec_t(), 0);

    // drop mark as true for empty blob
    jpgHvaBuf->drop = true;
    radarHvaBuf->drop = true;

    if (isEnd) {
        // last one in the batch. Mark it
        HVA_DEBUG("Set tag 1 on frame %d", blob->frameId);
        jpgHvaBuf->tagAs(1);
        radarHvaBuf->tagAs(1);
    } else {
        jpgHvaBuf->tagAs(0);
        radarHvaBuf->tagAs(0);
    }

    jpgHvaBuf->setMeta(meta);
    radarHvaBuf->setMeta(meta);
    blob->push(jpgHvaBuf);
    blob->push(radarHvaBuf);

    sendOutput(blob, 0, std::chrono::milliseconds(0));
}

#ifdef HVA_NODE_COMPILE_TO_DYNAMIC_LIBRARY
HVA_ENABLE_DYNAMIC_LOADING(LocalMultiSensorInputNode, LocalMultiSensorInputNode(threadNum))
#endif //#ifdef HVA_NODE_COMPILE_TO_DYNAMIC_LIBRARY

}

}

}
