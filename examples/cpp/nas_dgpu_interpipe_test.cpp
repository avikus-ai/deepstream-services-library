/*
The MIT License

Copyright (c) 2021, Prominence AI, Inc.

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

//-------------------------------------------------------------------------------------
//
// This script demonstrates how to run multple Pipelines, each with an Interpipe
// Source, both listening to the same Interpipe Sink.
//
// A single Player is created with a File Source and Interpipe Sink. Two Inference
// Pipelines are created to listen to the single Player. 
//
// The two Pipelines can be created with different configs, models, and/or Trackers
// for side-by-side comparison. Both Pipelines run in their own main-loop with their 
// own main-context and have their own Window Sink for viewing and external control.

#include <iostream> 
#include <glib.h>
#include <vector>

#include "DslApi.h"
#include "yaml.hpp"

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


Yaml::Node root;

std::wstring str2wstr(const std::string &key)
{
    auto suri = root[key].As<std::string>();
    return std::wstring(suri.begin(), suri.end());
}


std::wstring file_path;
std::wstring primary_infer_config_file_1;
std::wstring primary_infer_config_file_2;
std::wstring primary_model_engine_file_1;
std::wstring primary_model_engine_file_2;
std::wstring tracker_config_file_1;
std::wstring tracker_config_file_2;
std::wstring preproc_config_file_1;
std::wstring preproc_config_file_2;
uint class_agnostic_1;
uint class_agnostic_2;
uint preprocessing_1;
uint preprocessing_2;
uint postprocessing_1;
uint postprocessing_2;
std::wstring postprocess_1;
std::wstring postprocess_2;
std::wstring match_metric_1;
std::wstring match_metric_2;
float match_threshold_1;
float match_threshold_2;
int num_labels;
int interval_1;
int interval_2;
std::wstring trk_method_1;
std::wstring trk_method_2;
uint sink_width;
uint sink_height;
int trk_width_1;
int trk_width_2;
int trk_height_1;
int trk_height_2;
int batch_size;
uint perf;

void parse_yaml(void) 
{
    file_path = str2wstr("file_path");
    primary_infer_config_file_1 = str2wstr("primary_infer_config_file_1");
    primary_infer_config_file_2 = str2wstr("primary_infer_config_file_2");
    primary_model_engine_file_1 = str2wstr("primary_model_engine_file_1");
    primary_model_engine_file_2 = str2wstr("primary_model_engine_file_2");
    tracker_config_file_1 = str2wstr("tracker_config_file_1");
    tracker_config_file_2 = str2wstr("tracker_config_file_2");
    preproc_config_file_1 = str2wstr("preprocess_config_file_1");
    preproc_config_file_2 = str2wstr("preprocess_config_file_2");
    class_agnostic_1 = root["class_agnostic_1"].As<bool>();
    class_agnostic_2 = root["class_agnostic_2"].As<bool>();
    preprocessing_1 = root["preprocessing_1"].As<bool>();
    preprocessing_2 = root["preprocessing_2"].As<bool>();
    postprocessing_1 = root["postprocessing_1"].As<bool>();
    postprocessing_2 = root["postprocessing_2"].As<bool>();
    postprocess_1 = str2wstr("postprocess_1");
    postprocess_2 = str2wstr("postprocess_2");
    match_metric_1 = str2wstr("match_metric_1");
    match_metric_2 = str2wstr("match_metric_2");
    match_threshold_1 = root["match_threshold_1"].As<float>();
    match_threshold_2 = root["match_threshold_2"].As<float>();
    num_labels = root["num_labels"].As<int>();
    interval_1 = root["interval_1"].As<int>();
    interval_2 = root["interval_2"].As<int>();
    trk_method_1 = str2wstr("trk_method_1");
    trk_method_2 = str2wstr("trk_method_2");
    sink_width = root["sink_width"].As<int>();
    sink_height = root["sink_height"].As<int>();
    trk_width_1 = root["trk_width_1"].As<int>();
    trk_width_2 = root["trk_width_2"].As<int>();
    trk_height_1 = root["trk_height_1"].As<int>();
    trk_height_2 = root["trk_height_2"].As<int>();
    batch_size = root["batch_size"].As<int>();
    perf = root["perf"].As<bool>();
}

// File path for the single File Source
// static const std::wstring file_path(
//     L"/opt/nvidia/deepstream/deepstream/samples/streams/sample_qHD.mp4");

// Filespecs for the Primary GIE
// static const std::wstring primary_infer_config_file_1(
//     L"/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary_nano.txt");
// static const std::wstring primary_infer_config_file_2(
//     // L"../../test/configs/config_infer_primary_nano_nms_test.txt");
//     L"/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary_nano.txt");
     
// static const std::wstring primary_model_engine_file(
//     L"/opt/nvidia/deepstream/deepstream/samples/models/Primary_Detector_Nano/resnet10.caffemodel_b8_gpu0_fp16.engine");
// static const std::wstring tracker_config_file(
//     L"/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_IOU.yml");

// File name for .dot file output
static const std::wstring dot_file = L"state-playing";


GThread* main_loop_thread_1(NULL);
GThread* main_loop_thread_2(NULL);

uint g_num_active_pipelines = 0;

//     
// Objects of this class will be used as "client_data" for all callback notifications.
// Defines a class of all component names associated with a single Pipeline.     
// The names are derived from the provided unique id     
//    
struct ClientData
{
    ClientData(uint id){
        pipeline = L"pipeline-" + std::to_wstring(id);
        source = L"source-" + std::to_wstring(id);    
        pgie = L"pgie-" + std::to_wstring(id);
        preprocessor = L"preprocessor-" + std::to_wstring(id);
        tracker = L"tracker-" + std::to_wstring(id);
        osd = L"osd-" + std::to_wstring(id);
        window_sink = L"window-sink-" + std::to_wstring(id);    
    }

    std::wstring pipeline;
    std::wstring source;
    std::wstring preprocessor;
    std::wstring pgie;
    std::wstring tracker;
    std::wstring osd;
    std::wstring window_sink;
};

// Client data for two Pipelines
ClientData client_data_1(1);
ClientData client_data_2(2);

// Prototypes
DslReturnType create_pipeline(ClientData* client_data);
DslReturnType delete_pipeline(ClientData* client_data);

// 
// Function to be called on XWindow KeyRelease event
// 
static void xwindow_key_event_handler(const wchar_t* in_key, void* client_data)
{   
    std::wcout << L"key released = " << in_key << std::endl;

    std::wstring wkey(in_key); 
    std::string key(wkey.begin(), wkey.end());

    // ClientData* c_data = (ClientData*) client_data;
    ClientData* c_data1 = &client_data_1;
    ClientData* c_data2 = &client_data_2;
    
    key = std::toupper(key[0]);
    if(key == "P"){
        dsl_pipeline_pause(c_data1->pipeline.c_str());
        dsl_pipeline_pause(c_data2->pipeline.c_str());
    } else if(key == "S"){
        dsl_pipeline_stop(c_data1->pipeline.c_str());
        dsl_pipeline_stop(c_data2->pipeline.c_str());
    } else if (key == "R"){
        dsl_pipeline_play(c_data1->pipeline.c_str());
        dsl_pipeline_play(c_data2->pipeline.c_str());
    } else if (key == "Q"){
        std::wcout << L"Pipeline Quit" << std::endl;

        // quiting the main loop will allow the pipeline thread to 
        // stop and delete the pipeline and its components
        dsl_pipeline_main_loop_quit(c_data1->pipeline.c_str());
        dsl_pipeline_main_loop_quit(c_data2->pipeline.c_str());
    }
}

// 
// Function to be called on XWindow Delete event
//
static void xwindow_delete_event_handler(void* client_data)
{
    ClientData* c_data = (ClientData*)client_data; 
    std::wcout << L"delete window event for Pipeline " 
        << c_data->pipeline.c_str() << std::endl;

    // quiting the main loop will allow the pipeline thread to 
    // stop and delete the pipeline and its components
    dsl_pipeline_main_loop_quit(c_data->pipeline.c_str());
}

// 
// Function to be called on End-of-Stream (EOS) event
// 
static void eos_event_listener(void* client_data)
{
    ClientData* c_data = (ClientData*)client_data; 
    std::wcout << L"EOS event for Pipeline " 
        << c_data->pipeline.c_str() << std::endl;

    // quiting the main loop will allow the pipeline thread to 
    // stop and delete the pipeline and its components
    dsl_pipeline_main_loop_quit(c_data->pipeline.c_str());
}    

// 
// Function to be called on every change of Pipeline state
// 
static void state_change_listener(uint old_state, uint new_state, void* client_data)
{
    std::wcout << L"previous state = " << dsl_state_value_to_string(old_state) 
        << L", new state = " << dsl_state_value_to_string(new_state) << std::endl;
}

DslReturnType create_pipeline(ClientData* client_data)
{
    DslReturnType retval(DSL_RESULT_SUCCESS);

    // New File Source using the same URI for all Piplines
    retval = dsl_source_file_new(client_data->source.c_str(),
        file_path.c_str(), false);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    // New OSD with text, clock and bbox display all enabled. 
    retval = dsl_osd_new(client_data->osd.c_str(), true, true, true, false);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    // New Window Sink using the global dimensions
    retval = dsl_sink_window_new(client_data->window_sink.c_str(),
        0, 0, sink_width, sink_height);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    const wchar_t* component_names[] = 
    {
        client_data->source.c_str(), client_data->osd.c_str(), 
        client_data->window_sink.c_str(), NULL
    };

    retval = dsl_pipeline_new_component_add_many(client_data->pipeline.c_str(),
        component_names);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    // Add the XWindow event handler functions defined above
    retval = dsl_pipeline_xwindow_key_event_handler_add(client_data->pipeline.c_str(), 
        xwindow_key_event_handler, (void*)client_data);
    if (retval != DSL_RESULT_SUCCESS) return retval;
    
    retval = dsl_pipeline_xwindow_delete_event_handler_add(
        client_data->pipeline.c_str(), xwindow_delete_event_handler, 
        (void*)client_data);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    // Add the listener callback functions defined above
    retval = dsl_pipeline_state_change_listener_add(client_data->pipeline.c_str(), 
        state_change_listener, (void*)client_data);
    if (retval != DSL_RESULT_SUCCESS) return retval;
    
    retval = dsl_pipeline_eos_listener_add(client_data->pipeline.c_str(), 
        eos_event_listener, (void*)client_data);
    if (retval != DSL_RESULT_SUCCESS) return retval;
    
    // Tell the Pipeline to create its own main-context and main-loop that
    // will be set as the default main-context for the main_loop_thread_func
    // defined below once it is run. 
    retval = dsl_pipeline_main_loop_new(client_data->pipeline.c_str());

    return retval;    
}

DslReturnType delete_pipeline(ClientData* client_data)
{
    DslReturnType retval(DSL_RESULT_SUCCESS);

    std::wcout << L"stoping and deleting Pipeline " 
        << client_data->pipeline.c_str() << std::endl;
        
    // Stop the pipeline
    retval = dsl_pipeline_stop(client_data->pipeline.c_str());
    if (retval != DSL_RESULT_SUCCESS) return retval;

    // Delete the Pipeline first, then the components. 
    retval = dsl_pipeline_delete(client_data->pipeline.c_str());
    if (retval != DSL_RESULT_SUCCESS) return retval;

    const wchar_t* component_names[] = 
        {client_data->source.c_str(), client_data->window_sink.c_str(), NULL};

    // Now safe to delete all components for this Pipeline
    retval = dsl_component_delete_many(component_names);
    if (retval != DSL_RESULT_SUCCESS) return retval;

    std::wcout << L"Pipeline " 
        << client_data->pipeline.c_str() << " deleted successfully" << std::endl;
        
    g_num_active_pipelines--;
    
    if (!g_num_active_pipelines)
    {
        dsl_main_loop_quit();
    }
        
    return retval;    
}

//
// Thread function to start and wait on the main-loop
//
void* main_loop_thread_func(void *data)
{
    ClientData* client_data = (ClientData*)data;

    // Play the pipeline
    DslReturnType retval = dsl_pipeline_play(client_data->pipeline.c_str());
    if (retval != DSL_RESULT_SUCCESS) return NULL;

    g_num_active_pipelines++;

    // blocking call
    dsl_pipeline_main_loop_run(client_data->pipeline.c_str());
    
    delete_pipeline(client_data);

    return NULL;
}

int main(int argc, char** argv)
{  
    DslReturnType retval(DSL_RESULT_SUCCESS);

    Yaml::Parse(root, "nas_dgpu_interpipe_test.yml");
    
    parse_yaml();

    
    // # Since we're not using args, we can Let DSL initialize GST on first call
    while(true)
    {
        // New file source using the filespath defined above
        retval = dsl_source_file_new(L"file-source", file_path.c_str(), false);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New interpipe sink to broadcast to all listeners (interpipe sources).
        retval = dsl_sink_interpipe_new(L"interpipe-sink", true, true);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New Player to play the file source with interpipe sink
        retval = dsl_player_new(L"player", L"file-source", L"interpipe-sink");
        if (retval != DSL_RESULT_SUCCESS) break;
    
        // --------------------------------------------------------------------------
        // Create the first Pipeline with common components 
        // - interpipe source, OSD, and window sink,
        // - then add PGIE and Tracker using first set of configs
        retval = create_pipeline(&client_data_1);
        if (retval != DSL_RESULT_SUCCESS) break;

        if (preprocessing_1) {
            // New Preprocessor component using the config filespec defined above.
            retval = dsl_preproc_new(L"preprocessor-1", preproc_config_file_1.c_str());
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(client_data_1.pipeline.c_str(), L"preprocessor-1");
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        // New Primary GIE using the first config file. 
        retval = dsl_infer_gie_primary_new(client_data_1.pgie.c_str(),
            primary_infer_config_file_1.c_str(), primary_model_engine_file_1.c_str(), interval_1);
        if (retval != DSL_RESULT_SUCCESS) break;

        if (preprocessing_1) {
            // **** IMPORTANT! for best performace we explicity set the GIE's batch-size 
            // to the number of ROI's defined in the Preprocessor configuraton file.
            retval = dsl_infer_batch_size_set(client_data_1.pgie.c_str(), batch_size);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            // **** IMPORTANT! we must set the input-meta-tensor setting to true when
            // using the preprocessor, otherwise the GIE will use its own preprocessor.
            retval = dsl_infer_gie_tensor_meta_settings_set(client_data_1.pgie.c_str(),
                true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        // New IOU Tracker, setting max width and height of input frame
        if(trk_method_1 == L"IOU") {
            retval = dsl_tracker_iou_new(client_data_1.tracker.c_str(), 
                tracker_config_file_1.c_str(), trk_width_1, trk_height_1);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else if(trk_method_1 == L"DCF") {
            retval = dsl_tracker_iou_new(client_data_1.tracker.c_str(), 
                tracker_config_file_1.c_str(), trk_width_1, trk_height_1);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        if (postprocessing_1) {
            auto param1 = (postprocess_1 == L"NMM") ? DSL_NMP_PROCESS_METHOD_MERGE : DSL_NMP_PROCESS_METHOD_SUPRESS;
            auto param2 = (match_metric_1 == L"IOS") ? DSL_NMP_MATCH_METHOD_IOS : DSL_NMP_MATCH_METHOD_IOU;
            // Create a new Non Maximum Processor (NMP) Pad Probe Handler (PPH). 
            retval = dsl_pph_nmp_new(L"nmp-pph-1", nullptr, param1, param2, match_threshold_1);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_tracker_pph_add(client_data_1.tracker.c_str(), L"nmp-pph-1", DSL_PAD_SINK);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        const wchar_t* additional_components_1[] = 
            {client_data_1.pgie.c_str(), client_data_1.tracker.c_str(), NULL};

        // Add the new components to the first Pipeline
        retval = dsl_pipeline_component_add_many(client_data_1.pipeline.c_str(),
            additional_components_1);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Start the Pipeline with its own main-context and main-loop in a 
        // seperate thread. 
        main_loop_thread_1 = g_thread_new("main-loop-1", 
            main_loop_thread_func, &client_data_1);

        // --------------------------------------------------------------------------
        // Create the Second Pipeline with common components 
        // - interpipe source, OSD, and window sink,
        // - then add PGIE and Tracker using second set of configs
        
        retval = create_pipeline(&client_data_2);
        if (retval != DSL_RESULT_SUCCESS) break;

        if (preprocessing_2) {
            // New Preprocessor component using the config filespec defined above.
            retval = dsl_preproc_new(L"preprocessor-2", preproc_config_file_2.c_str());
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(client_data_2.pipeline.c_str(), L"preprocessor-2");
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        // New Primary GIE using the second config file. 
        retval = dsl_infer_gie_primary_new(client_data_2.pgie.c_str(),
            primary_infer_config_file_2.c_str(), primary_model_engine_file_2.c_str(), interval_2);
        if (retval != DSL_RESULT_SUCCESS) break;

        if (preprocessing_2) {
            // **** IMPORTANT! for best performace we explicity set the GIE's batch-size 
            // to the number of ROI's defined in the Preprocessor configuraton file.
            retval = dsl_infer_batch_size_set(client_data_2.pgie.c_str(), batch_size);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            // **** IMPORTANT! we must set the input-meta-tensor setting to true when
            // using the preprocessor, otherwise the GIE will use its own preprocessor.
            retval = dsl_infer_gie_tensor_meta_settings_set(client_data_2.pgie.c_str(),
                true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        // New IOU Tracker, setting max width and height of input frame
        if(trk_method_2 == L"IOU") {
            retval = dsl_tracker_iou_new(client_data_2.tracker.c_str(), 
                tracker_config_file_2.c_str(), trk_width_2, trk_height_2);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else if(trk_method_2 == L"DCF") {
            retval = dsl_tracker_iou_new(client_data_2.tracker.c_str(), 
                tracker_config_file_2.c_str(), trk_width_2, trk_height_2);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        if (postprocessing_2) {
            auto param1 = (postprocess_2 == L"NMM") ? DSL_NMP_PROCESS_METHOD_MERGE : DSL_NMP_PROCESS_METHOD_SUPRESS;
            auto param2 = (match_metric_2 == L"IOS") ? DSL_NMP_MATCH_METHOD_IOS : DSL_NMP_MATCH_METHOD_IOU;
            // Create a new Non Maximum Processor (NMP) Pad Probe Handler (PPH). 
            retval = dsl_pph_nmp_new(L"nmp-pph-2", nullptr, param1, param2, match_threshold_2);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_tracker_pph_add(client_data_2.tracker.c_str(), L"nmp-pph-2", DSL_PAD_SINK);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        const wchar_t* additional_components_2[] = 
            {client_data_2.pgie.c_str(), client_data_2.tracker.c_str(), NULL};
        
        // Add the new components to the second Pipeline
        retval = dsl_pipeline_component_add_many(client_data_2.pipeline.c_str(),
            additional_components_2);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        // Start the Pipeline with its own main-context and main-loop in a 
        // seperate thread. 
        main_loop_thread_2 = g_thread_new("main-loop-2", 
            main_loop_thread_func, &client_data_2);

        // --------------------------------------------------------------------------
        // Once the Pipelines are running we can start the player - i.e common stream
        retval = dsl_player_play(L"player");
        
        // Join both threads - in any order.
        g_thread_join(main_loop_thread_1);
        g_thread_join(main_loop_thread_2);
        
        break;
    }

    // Print out the final result
    std::wcout << dsl_return_value_to_string(retval) << std::endl;

    dsl_delete_all();

    std::cout << "Goodbye!" << std::endl;  
    return 0;
}
