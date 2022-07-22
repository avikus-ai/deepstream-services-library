/*
The MIT License

Copyright (c) 2022, Prominence AI, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in-
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <iostream>
#include <glib.h>
#include <gst/gst.h>
#include <gstnvdsmeta.h>
#include <nvdspreprocess_meta.h>
#include <algorithm>
#include <numeric>
// https://github.com/dpilger26/NumCpp/blob/master/docs/markdown/Installation.md
#include <NumCpp.hpp>
#include <cstdint>
#include <unordered_map>

#include "DslApi.h"

std::wstring uri_h265(
    L"/opt/nvidia/deepstream/deepstream/samples/streams/sample_qHD.mp4");

// Config file used with the Preprocessor
// roi-params-src-0=0;0;1920;1080;0;300;896;504;716;300;896;504;1024;300;896;504
// network-input-shape= 4;3;368;640
// Important - comment property [Group 1],[Group 2]
std::wstring preproc_config(
    L"/opt/dsl/dvcData/cuda114/config_preprocess_v5s.txt");

// Config and model-engine files 
std::wstring primary_infer_config_file(
    L"/opt/dsl/dvcData/cuda114/config_infer_primary_yoloV5s.txt");
std::wstring primary_model_engine_file(
    L"/opt/dsl/dvcData/cuda114/model_b4_gpu0_fp16.engine");
    // L"/opt/dsl/dvcData/cuda114/5s_640_model_b4_gpu0_fp16.engine");

// Config file used by the IOU Tracker    
std::wstring tracker_config_file(
    L"/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_IOU.yml");

uint VECTOR_RESERVE_SIZE = 1000;
uint CLASS_AGNOSTIC = false;
std::wstring MATCH_METRIC = L"IOS"; // IOU, IOS 
float MATCH_THRESHOLD = 0.5;
int num_labels;

uint WINDOW_WIDTH = DSL_DEFAULT_STREAMMUX_WIDTH;
uint WINDOW_HEIGHT = DSL_DEFAULT_STREAMMUX_HEIGHT;

class ReportData {
public:
    int m_report_count;
    int m_header_interval;

    ReportData(int count, int interval) : m_report_count(count), m_header_interval(interval) {}
};

boolean dsl_pph_meter_cb(double* session_fps_averages, double* interval_fps_averages, 
    uint source_count, void* client_data)
{
    // cast the C void* client_data back to a py_object pointer and deref
    // report_data = cast(client_data, POINTER(py_object)).contents.value
    ReportData *report_data = static_cast<ReportData*>(client_data);

    // Print header on interval
    if (report_data->m_report_count % report_data->m_header_interval == 0) {
        std::wstring header = L"";

        for(int i=0; i<source_count; i++) {  
            header += L"FPS ";
            header += std::to_wstring(i);
            header += L" (AVG)";
        }
        std::wcout << header << "\n";
    }
    
    // Print FPS counters
    std::wstring counters = L"";

    for(int i=0; i<source_count; i++) {
        counters += std::to_wstring(interval_fps_averages[i]);
        counters += L" ";
        counters += std::to_wstring(session_fps_averages[i]);
    }

    std::wcout << counters << "\n";

    // Increment reporting count
    report_data->m_report_count += 1;
    
    return true;
}

// 
// Function to be called on XWindow KeyRelease event
// 
void xwindow_key_event_handler(const wchar_t* in_key, void* client_data)
{   
    std::wstring wkey(in_key); 
    std::string key(wkey.begin(), wkey.end());
    std::cout << "key released = " << key << std::endl;
    key = std::toupper(key[0]);
    if(key == "P"){
        dsl_pipeline_pause(L"pipeline");
    } else if (key == "R"){
        dsl_pipeline_play(L"pipeline");
    } else if (key == "Q" or key == "" or key == ""){
        std::cout << "Main Loop Quit" << std::endl;
        dsl_pipeline_stop(L"pipeline");
        dsl_main_loop_quit();
    }
}

// 
// Function to be called on XWindow Delete event
//
void xwindow_delete_event_handler(void* client_data)
{
    std::cout<<"delete window event"<<std::endl;
    dsl_pipeline_stop(L"pipeline");
    dsl_main_loop_quit();
}
    
// 
// Function to be called on End-of-Stream (EOS) event
// 
void eos_event_listener(void* client_data)
{
    std::cout<<"Pipeline EOS event"<<std::endl;
    dsl_pipeline_stop(L"pipeline");
    dsl_main_loop_quit();
}    

// 
// Function to be called on every change of Pipeline state
// 
void state_change_listener(uint old_state, uint new_state, void* client_data)
{
    std::cout<<"previous state = " << dsl_state_value_to_string(old_state) 
        << ", new state = " << dsl_state_value_to_string(new_state) << std::endl;
}

template<typename T>
void print_1dVector(const std::vector<T> &arr) {
	for( auto &row : arr) {
		std::cout << row << ' ';
	}
	std::cout << '\n';
}

template<typename T>
void print_2dVector(const std::vector<T> &arr) {
	for( auto &row : arr) {
		for(auto &col : row)
			 std::cout << col << ' ';
		std::cout << '\n';
	}
}

template<typename T>
std::vector<uint32_t> argsort(const std::vector<T> &array) {
    std::vector<uint32_t> indices(array.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&array](int left, int right) -> bool {
                  // sort indices according to corresponding array element
                  return array[left] < array[right];
              });

    return indices;
}

std::vector<float> calculate_box_union(const std::vector<float> &box1, const std::vector<float> &box2) {
    float x1 = std::min(box1[0], box2[0]);
    float y1 = std::min(box1[1], box2[1]);
    float x2 = std::max(box1[2], box2[2]);
    float y2 = std::max(box1[3], box2[3]);
    return std::vector<float>{x1, y1, x2, y2, box1[4]};
}

//
// Custom Pad Probe Handler (PPH) to process the batch-meta for every frame
//
uint custom_batch_meta_handler(void* buffer, void* client_data)
{
    GstBuffer* pGstBuffer = (GstBuffer*)buffer;
    
    NvDsBatchMeta* pBatchMeta = gst_buffer_get_nvds_batch_meta(pGstBuffer);
    
    // client_data -> Agnostic?

    // For each frame in the batched meta data
    for (NvDsMetaList* pFrameMetaList = pBatchMeta->frame_meta_list; 
        pFrameMetaList; pFrameMetaList = pFrameMetaList->next)
    {
        // Check for valid frame data
        NvDsFrameMeta* pFrameMeta = (NvDsFrameMeta*)(pFrameMetaList->data);
        if (pFrameMeta != NULL)
        {
            NvDsMetaList* pObjectMetaList = pFrameMeta->obj_meta_list;
			std::vector<std::vector<NvDsObjectMeta*>> obj_array;
			std::vector<std::vector<std::vector<float>>> predictions;

			// [fix] parse the label file			
			num_labels = CLASS_AGNOSTIC ? 1 : 80;
			predictions.resize(num_labels);
            obj_array.resize(num_labels);

            for (int lb=0; lb<num_labels; lb++) {
                predictions[lb].reserve(VECTOR_RESERVE_SIZE);
                obj_array[lb].reserve(VECTOR_RESERVE_SIZE);
            }

            // For each detected object in the frame.
            while (pObjectMetaList)
            {
                // Check for valid object data
                NvDsObjectMeta* pObjectMeta = (NvDsObjectMeta*)(pObjectMetaList->data);

                // Important - we need to advance the list pointer before removing.
                pObjectMetaList = pObjectMetaList->next;

                if (pObjectMeta != NULL)
                {
					// https://github.com/obss/sahi/blob/91a0becb0d86f0943c57f966a86a845a70c0eb77/sahi/postprocess/combine.py#L17-L40
					if (CLASS_AGNOSTIC) {
                        obj_array[0].emplace_back(pObjectMeta);
						predictions[0].emplace_back(std::vector<float>{
									pObjectMeta->rect_params.left,
									pObjectMeta->rect_params.top,
									pObjectMeta->rect_params.left + pObjectMeta->rect_params.width,
									pObjectMeta->rect_params.top + pObjectMeta->rect_params.height, 
									pObjectMeta->confidence,
									});
					}
					else {
                        obj_array[pObjectMeta->class_id].emplace_back(pObjectMeta);
						predictions[pObjectMeta->class_id].emplace_back(std::vector<float>{
                                     pObjectMeta->rect_params.left,
                                     pObjectMeta->rect_params.top,
                                     pObjectMeta->rect_params.left + pObjectMeta->rect_params.width,
                                     pObjectMeta->rect_params.top + pObjectMeta->rect_params.height, 
                                     pObjectMeta->confidence,
                                     });

					}
                    // nvds_remove_obj_meta_from_frame(pFrameMeta, pObjectMeta);
                }
            }
			
			for (int lb=0; lb<num_labels; lb++) {
                // print_2dVector(predictions);							
                // https://dpilger26.github.io/NumCpp/doxygen/html/classnc_1_1_nd_array.html#a9d7045ecdff86bac3306a8bfd9a787eb
                if (predictions[lb].size() == 0)
                    continue;

                // keep_to_merge_list = {}
                std::unordered_map<int, std::vector<int>> keep_to_merge_list;

                nc::NdArray<float> nd_predictions{predictions[lb]};

    			// # we extract coordinates for every
    			// # prediction box present in P
       		    // x1 = predictions[:, 0]
    			// y1 = predictions[:, 1]
    			// x2 = predictions[:, 2]
    			// y2 = predictions[:, 3]
                
                auto x1 = nd_predictions(nd_predictions.rSlice(), 0);
                auto y1 = nd_predictions(nd_predictions.rSlice(), 1);
                auto x2 = nd_predictions(nd_predictions.rSlice(), 2);
                auto y2 = nd_predictions(nd_predictions.rSlice(), 3);
                
                // scores = predictions[:, 4]
                auto scores = nd_predictions(nd_predictions.rSlice(), 4);

                // areas = (x2 - x1) * (y2 - y1)
                auto areas = (x2 - x1) * (y2 - y1);

                // std::vector<size_t> order = argsort(obj_array);
                // https://dpilger26.github.io/NumCpp/doxygen/html/classnc_1_1_nd_array.html#a1fb3a21ab9c10a2684098df919b5b440
                
                // # sort the prediction boxes in P
                // # according to their confidence scores
                // order = scores.argsort()
                std::vector<uint32_t> order = argsort(scores.toStlVector());
                
                nc::NdArray<nc::uint32> nd_order{order};
                // std::sort(obj_array.begin(), obj_array.end(), confidence_compare);
                
                // # initialise an empty list for
                // # filtered prediction boxes
                // keep = []

                std::vector<unsigned int> keep, remove;
                
                // return DSL_PAD_PROBE_OK;

                //while (order.size() > 0) {
                while (nc::shape(nd_order).size() > 0) {
                    // auto idx = order.back();
                    auto idx = nd_order[-1];
                    
                    // order.pop_back();
                    nd_order = nd_order(0, nc::Slice(0,-1));
                    if (nc::shape(nd_order).size() == 0) 
                        break;	
                    
    //				# select coordinates of BBoxes according to
    //		        # the indices in order
    //        		xx1 = torch.index_select(x1, dim=0, index=order)
    //        		xx2 = torch.index_select(x2, dim=0, index=order)
    //        		yy1 = torch.index_select(y1, dim=0, index=order)
    //        		yy2 = torch.index_select(y2, dim=0, index=order)
                    
                    nc::NdArray<nc::uint32> index_other = nd_order;
                    nc::NdArray<nc::uint32> index{idx};
                    
                    auto xx1 = x1[index_other];
                    auto xx2 = x2[index_other];
                    auto yy1 = y1[index_other];
                    auto yy2 = y2[index_other];

    //				# find the coordinates of the intersection boxes
    //				xx1 = torch.max(xx1, x1[idx])
    //				yy1 = torch.max(yy1, y1[idx])
    //				xx2 = torch.min(xx2, x2[idx])
    //				yy2 = torch.min(yy2, y2[idx])

                    for(auto it = xx1.begin(); it != xx1.end(); ++it) 
                        if (*it < x1[index].item()) 
                            *it = x1[index].item();

                    for(auto it = yy1.begin(); it != yy1.end(); ++it) 
                        if (*it < y1[index].item()) 
                            *it = y1[index].item();
                                    
                    for(auto it = xx2.begin(); it != xx2.end(); ++it) 
                        if (*it > x2[index].item()) 
                            *it = x2[index].item();

                    for(auto it = yy2.begin(); it != yy2.end(); ++it) 
                        if (*it > y2[index].item()) 
                            *it = y2[index].item();

    //				# find height and width of the intersection boxes
    //				w = xx2 - xx1
    //				h = yy2 - yy1
                    
                    auto w = xx2 - xx1;
                    auto h = yy2 - yy1;
                    
    //				# take max with 0.0 to avoid negative w and h
    //				# due to non-overlapping boxes
    //				w = torch.clamp(w, min=0.0)
    //				h = torch.clamp(h, min=0.0)
                    
                    w = nc::clip(w, 0.0f, float(1e9));
                    h = nc::clip(h, 0.0f, float(1e9));
                    
    //				# find the intersection area
    //		        inter = w * h
                    
                    auto inter = w * h;
                    
    //				# find the areas of BBoxes according the indices in order
    //        		rem_areas = torch.index_select(areas, dim=0, index=order)i
                    
                    auto rem_areas = areas[index_other];
                    
                    if (MATCH_METRIC == L"IOU") {
    //					if match_metric == "IOU":
    //					# find the union of every prediction T in P
    //					# with the prediction S
    //					# Note that areas[idx] represents area of S
    //					union = (rem_areas - inter) + areas[idx]
    //					# find the IoU of every prediction in P with S
    //					match_metric_value = inter / union
                        
                        auto _union = (rem_areas - inter) + areas[index].item();
                        auto match_metric_value = inter / _union;

                        // mask = match_metric_value < match_threshold
                        auto mask = match_metric_value < MATCH_THRESHOLD;					
                        
                        auto rm_idx = 0;
                        for(auto it = mask.begin(); it != mask.end(); ++it, ++rm_idx) {
                            if (*it == 0) {
                                keep_to_merge_list[idx].emplace_back(index_other[rm_idx]);
                                remove.emplace_back(index_other[rm_idx]);
                            }
                        }

                        nd_order = nd_order[mask];
                    }
                    else if (MATCH_METRIC == L"IOS") {
    					// # find the smaller area of every prediction T in P
    					// # with the prediction S
    					// # Note that areas[idx] represents area of S
    					// smaller = torch.min(rem_areas, areas[idx])
    					// # find the IoU of every prediction in P with S
    					// match_metric_value = inter / smaller
                        
                        auto smaller = rem_areas;
                        for(auto it = smaller.begin(); it != smaller.end(); ++it)
                            if (*it > areas[index].item())
                                *it = areas[index].item();					
                        auto match_metric_value = inter / smaller;

                        auto mask = match_metric_value < MATCH_THRESHOLD;					
                        
                        auto rm_idx = 0;
                        for(auto it = mask.begin(); it != mask.end(); ++it, ++rm_idx) {
                            if (*it == 0) {
                                // for matched_box_ind in matched_box_indices.tolist():
                                //     keep_to_merge_list[idx.tolist()].append(matched_box_ind)
                                keep_to_merge_list[idx].emplace_back(index_other[rm_idx]);
                                remove.emplace_back(index_other[rm_idx]);
                            }
                        }

                        nd_order = nd_order[mask];
                    }
                    else {
                        return -1; // Change assert
                    }			
                }

                // selected_object_predictions = []
                // for keep_ind, merge_ind_list in keep_to_merge_list.items():
                //     for merge_ind in merge_ind_list:
                //         if has_match(
                //             object_prediction_list[keep_ind].tolist(),
                //             object_prediction_list[merge_ind].tolist(),
                //             self.match_metric,
                //             self.match_threshold,
                //         ):
                //             object_prediction_list[keep_ind] = merge_object_prediction_pair(
                //                 object_prediction_list[keep_ind].tolist(), object_prediction_list[merge_ind].tolist()
                //             )
                //     selected_object_predictions.append(object_prediction_list[keep_ind].tolist())

                for (auto it = keep_to_merge_list.begin(); it != keep_to_merge_list.end(); ++it) {
                    // std::cout << it->first    // string (key)
                    //         << ": [ ";
                    // for (auto &x : it->second) {
                    //     std::cout << x << ' ';
                    // }
                    // std::cout << "]\n";
                    for (auto &merge_ind : it->second)
                        predictions[lb][it->first] = calculate_box_union(predictions[lb][it->first], predictions[lb][merge_ind]);

                    obj_array[lb][it->first]->rect_params.left = predictions[lb][it->first][0];
                    obj_array[lb][it->first]->rect_params.top = predictions[lb][it->first][1];
                    obj_array[lb][it->first]->rect_params.width = predictions[lb][it->first][2] - predictions[lb][it->first][0];
                    obj_array[lb][it->first]->rect_params.height = predictions[lb][it->first][3] - predictions[lb][it->first][1]; 
                }

                for (auto &x: remove) {
                    nvds_remove_obj_meta_from_frame(pFrameMeta, obj_array[lb][x]);
                }
			}
		
        }
    }

    return DSL_PAD_PROBE_OK;
}

int main(int argc, char** argv)
{
    DslReturnType retval = DSL_RESULT_FAILURE;

    // Since we're not using args, we can Let DSL initialize GST on first call    
    while(true) 
    {    
        ReportData report_data(0, 12);

        retval = dsl_pph_meter_new(L"meter-pph", 1, dsl_pph_meter_cb, &report_data);
        if (retval != DSL_RESULT_SUCCESS) break;

        //```````````````````````````````````````````````````````````````````````````````````
        // Create a new Custom Pad Probe Handler. 
        retval = dsl_pph_custom_new(L"custom_pph",
            custom_batch_meta_handler, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New File Source
        retval = dsl_source_file_new(L"uri-source-1", uri_h265.c_str(), true);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New Preprocessor component using the config filespec defined above.
        retval = dsl_preproc_new(L"preprocessor", preproc_config.c_str());
        if (retval != DSL_RESULT_SUCCESS) break;

        // New Primary GIE using the filespecs defined above, with interval and Id
        retval = dsl_infer_gie_primary_new(L"primary-gie", 
            primary_infer_config_file.c_str(), primary_model_engine_file.c_str(), 0);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        // **** IMPORTANT! for best performace we explicity set the GIE's batch-size 
        // to the number of ROI's defined in the Preprocessor configuraton file.
        // retval = dsl_infer_batch_size_set(L"primary-gie", 4);
        // if (retval != DSL_RESULT_SUCCESS) break;
        
        // **** IMPORTANT! we must set the input-meta-tensor setting to true when
        // using the preprocessor, otherwise the GIE will use its own preprocessor.
        retval = dsl_infer_gie_tensor_meta_settings_set(L"primary-gie",
            true, false);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New IOU Tracker, setting max width and height of input frame
        retval = dsl_tracker_iou_new(L"iou-tracker", 
            tracker_config_file.c_str(), 480, 272);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Add the custom PPH to the source pad of the Tracker
        // retval = dsl_tracker_pph_add(L"iou-tracker", L"custom_pph", DSL_PAD_SINK);
        // if (retval != DSL_RESULT_SUCCESS) break;

        // New OSD with text, clock and bbox display all enabled. 
        retval = dsl_osd_new(L"on-screen-display", true, true, true, false);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_osd_pph_add(L"on-screen-display", L"meter-pph", DSL_PAD_SINK);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New Overlay Sink, 0 x/y offsets and same dimensions as Tiled Display
        retval = dsl_sink_window_new(L"window-sink", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        if (retval != DSL_RESULT_SUCCESS) break;
    
        // Create a list of Pipeline Components to add to the new Pipeline.
        const wchar_t* components[] = {L"uri-source-1",  L"preprocessor", L"primary-gie", 
            L"iou-tracker", L"on-screen-display", L"window-sink", NULL};
        
        // Add all the components to our pipeline
        retval = dsl_pipeline_new_component_add_many(L"pipeline", components);
        if (retval != DSL_RESULT_SUCCESS) break;
            
        // Add the EOS listener and XWindow event handler functions defined above
        retval = dsl_pipeline_eos_listener_add(L"pipeline", eos_event_listener, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_pipeline_xwindow_key_event_handler_add(L"pipeline", 
            xwindow_key_event_handler, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_pipeline_xwindow_delete_event_handler_add(L"pipeline", 
            xwindow_delete_event_handler, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Play the pipeline
        retval = dsl_pipeline_play(L"pipeline");
        if (retval != DSL_RESULT_SUCCESS) break;

        // Start and join the main-loop
        dsl_main_loop_run();
        break;

    }
    
    // Print out the final result
    std::wcout << dsl_return_value_to_string(retval) << std::endl;

    dsl_delete_all();

    std::cout<<"Goodbye!"<<std::endl;  
    return 0;
}
            
