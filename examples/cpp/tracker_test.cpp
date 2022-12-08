#include <iostream>
#include <glib.h>
#include <X11/Xlib.h>

#include <gst/gst.h>
#include <gstnvdsmeta.h>

#include "DslApi.h"



// File path for the single File Source
std::wstring file_path1(
    L"/opt/dsl/nas_data/sample_video/deepsort_person_sample.mp4");

std::wstring primary_infer_config_file(
    L"/opt/dsl/nas_data/COCO_DGPU/config_infer_primary_yoloV5s.txt");
std::wstring primary_model_engine_file(
    L"/opt/dsl/nas_data/COCO_DGPU/model_b1_gpu0_fp16.engine");
std::wstring tracker_config_file(
    L"/opt/dsl/nas_data/COCO_DGPU/config_tracker_DeepSORT.yml");
// std::wstring tracker_config_file(
//     L"/opt/dsl/nas_data/FLL_DGPU/config_tracker_NvDCF_max_perf.yml");
// std::wstring tracker_config_file(
//     L"/opt/dsl/nas_data/FLL_DGPU/config_tracker_IOU.yml");

// File name for .dot file output
static const std::wstring dot_file = L"state-playing";

int TILER_WIDTH = DSL_STREAMMUX_1K_HD_WIDTH*3; 
int TILER_HEIGHT = DSL_STREAMMUX_1K_HD_HEIGHT;

// Window Sink Dimensions - used to create the sink, however, in this
// example the Pipeline XWindow service is called to enabled full-sreen
int WINDOW_WIDTH = DSL_STREAMMUX_1K_HD_WIDTH;
int WINDOW_HEIGHT = DSL_STREAMMUX_1K_HD_HEIGHT;

int SHOW_SOURCE_TIMEOUT = 3;

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
        dsl_pipeline_stop(L"pipeline");
        dsl_main_loop_quit();
    } 
}
 
//
// Function to be called on XWindow Button Press event
// 
void xwindow_button_event_handler(uint button, 
    int xpos, int ypos, void* client_data)
{
    std::cout << "button = ", button, " pressed at x = ", xpos, " y = ", ypos;
    
    if (button == Button1){
        // get the current XWindow dimensions - the XWindow was overlayed with our Window Sink
        uint width(0), height(0);
        
        if (dsl_pipeline_xwindow_dimensions_get(L"pipeline", 
            &width, &height) == DSL_RESULT_SUCCESS)
            
            // call the Tiler to show the source based on the x and y button cooridantes
            //and the current window dimensions obtained from the XWindow
            dsl_tiler_source_show_select(L"tiler", 
                xpos, ypos, width, height, SHOW_SOURCE_TIMEOUT);
    }
}
 
// ## 
// # Function to be called on XWindow Delete event
// ##
void xwindow_delete_event_handler(void* client_data)
{
    std::cout<<"delete window event"<<std::endl;

    dsl_pipeline_stop(L"pipeline");
    dsl_main_loop_quit();
}
    

// # Function to be called on End-of-Stream (EOS) event
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

uint filter_person(void* buffer, void* client_data)
{
    GstBuffer* pGstBuffer = (GstBuffer*)buffer;

    NvDsBatchMeta* pBatchMeta = gst_buffer_get_nvds_batch_meta(pGstBuffer);

    // For each frame in the batched meta data
    for (NvDsMetaList* pFrameMetaList = pBatchMeta->frame_meta_list;
        pFrameMetaList; pFrameMetaList = pFrameMetaList->next)
    {
        // Check for valid frame data
        NvDsFrameMeta* pFrameMeta = (NvDsFrameMeta*)(pFrameMetaList->data);
        if (pFrameMeta != nullptr)
        {
            NvDsMetaList* pObjectMetaList = pFrameMeta->obj_meta_list;

            // For each detected object in the frame.
            while (pObjectMetaList)
            {
                // int grid;
                // Check for valid object data
                NvDsObjectMeta* pObjectMeta = (NvDsObjectMeta*)(pObjectMetaList->data);
                
                int obj_class_id = static_cast<int>(pObjectMeta->class_id);

                if (obj_class_id > 0) {
                    nvds_remove_obj_meta_from_frame(pFrameMeta, pObjectMeta);
                }

                pObjectMetaList = pObjectMetaList->next;
            }
       }
    }

    return DSL_PAD_PROBE_OK;
}


int main(int argc, char** argv)
{  
    DslReturnType retval;

    // # Since we're not using args, we can Let DSL initialize GST on first call
    while(true)
    {   
        retval = dsl_pph_custom_new(L"filtering", filter_person, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_source_file_new(L"uri-source-1", file_path1.c_str(), true);
        if (retval != DSL_RESULT_SUCCESS) break;

        // // New Primary GIE using the filespecs above, with interval and Id
        retval = dsl_infer_gie_primary_new(L"primary-gie", 
            primary_infer_config_file.c_str(), primary_model_engine_file.c_str(), 0);
        if (retval != DSL_RESULT_SUCCESS) break;

        // // New IOU Tracker, setting max width and height of input frame
        retval = dsl_tracker_new(L"deepsort-tracker", 
            tracker_config_file.c_str(), 1920, 1088);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_tracker_pph_add(L"deepsort-tracker",
            L"filtering", DSL_PAD_SINK);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New OSD with text and bbox display enabled. 
        retval = dsl_osd_new(L"on-screen-display", true, false, true, false);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New Overlay Sink, 0 x/y offsets and same dimensions as Tiled Display
        retval = dsl_sink_window_new(L"window-sink", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Create a list of Pipeline Components to add to the new Pipeline.
        const wchar_t* components[] = {L"uri-source-1",
            L"primary-gie", L"deepsort-tracker",
            L"on-screen-display", L"window-sink", NULL};

        // Add all the components to our pipeline
        retval = dsl_pipeline_new_component_add_many(L"pipeline", components);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        // Add the EOS listener and XWindow event handler functions defined above
        retval = dsl_pipeline_eos_listener_add(L"pipeline", eos_event_listener, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_pipeline_xwindow_key_event_handler_add(L"pipeline", 
            xwindow_key_event_handler, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_pipeline_xwindow_button_event_handler_add(L"pipeline", 
            xwindow_button_event_handler, NULL);
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

    // # Print out the final result
    std::wcout << dsl_return_value_to_string(retval) << std::endl;

    dsl_delete_all();

    std::cout<<"Goodbye!"<<std::endl;  
    return 0;
}
