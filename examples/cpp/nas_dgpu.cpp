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
#include <iomanip>
// https://github.com/dpilger26/NumCpp/blob/master/docs/markdown/Installation.md
#include <cstdint>
#include <chrono>
#include "yaml.hpp"

#include "DslApi.h"

namespace YML_VARIABLE {
    uint vector_reserve_size;
    uint class_agnostic;
    uint preprocessing;
    uint tracking;
    uint postprocessing;
    std::wstring postprocess;
    std::wstring match_metric; // IOU, IOS 
    float match_threshold;
    int num_labels;
    int interval;
    std::wstring trk;
    int trk_width;
    int trk_height;
    uint on_display_screen;
    uint window_width;
    uint window_height;
    int font_size;
    int bbox_border_size;
    uint batch_size;
    uint perf;
    uint ode;
    uint monitoring;
    uint uri_cnt;
    uint repeat_video;
    uint logging;
}

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

// 
// Callback function for the ODE Monitor Action - illustrates how to
// dereference the ODE "info_ptr" and access the data fields.
// Note: you would normally use the ODE Print Action to print the info
// to the console window if that is the only purpose of the Action.
// 
void ode_occurrence_monitor(dsl_ode_occurrence_info* pInfo, void* client_data)
{
    std::wcout << "Trigger Name        : " << pInfo->trigger_name << "\n";
    std::cout << "  Unique Id         : " << pInfo->unique_ode_id << "\n";
    std::cout << "  NTP Timestamp     : " << pInfo->ntp_timestamp << "\n";
    std::cout << "  Source Data       : ------------------------" << "\n";
    std::cout << "    Id              : " << pInfo->source_info.source_id << "\n";
    std::cout << "    Batch Id        : " << pInfo->source_info.batch_id << "\n";
    std::cout << "    Pad Index       : " << pInfo->source_info.pad_index << "\n";
    std::cout << "    Frame           : " << pInfo->source_info.frame_num << "\n";
    std::cout << "    Width           : " << pInfo->source_info.frame_width << "\n";
    std::cout << "    Height          : " << pInfo->source_info.frame_height << "\n";
    std::cout << "    Infer Done      : " << pInfo->source_info.inference_done << "\n";

    if (pInfo->is_object_occurrence)
    {
        std::cout << "  Object Data       : ------------------------" << "\n";
        std::cout << "    Class Id        : " << pInfo->object_info.class_id << "\n";
        std::cout << "    Infer Comp Id   : " << pInfo->object_info.inference_component_id << "\n";
        std::cout << "    Tracking Id     : " << pInfo->object_info.tracking_id << "\n";
        std::cout << "    Label           : " << pInfo->object_info.label << "\n";
        std::cout << "    Persistence     : " << pInfo->object_info.persistence << "\n";
        std::cout << "    Direction       : " << pInfo->object_info.direction << "\n";
        std::cout << "    Infer Conf      : " << pInfo->object_info.inference_confidence << "\n";
        std::cout << "    Track Conf      : " << pInfo->object_info.tracker_confidence << "\n";
        std::cout << "    Left            : " << pInfo->object_info.left << "\n";
        std::cout << "    Top             : " << pInfo->object_info.top << "\n";
        std::cout << "    Width           : " << pInfo->object_info.width << "\n";
        std::cout << "    Height          : " << pInfo->object_info.height << "\n";
    }
    else
    {
        std::cout << "  Accumulative Data : ------------------------" << "\n";
        std::cout << "    Occurrences     : " << pInfo->accumulative_info.occurrences_total << "\n";
        std::cout << "    Occurrences In  : " << pInfo->accumulative_info.occurrences_total << "\n";
        std::cout << "    Occurrences Out : " << pInfo->accumulative_info.occurrences_total << "\n";
    }
    std::cout << "  Trigger Criteria  : ------------------------" << "\n";
    std::cout << "    Class Id        : " << pInfo->criteria_info.class_id << "\n";
    std::cout << "    Infer Comp Id   : " << pInfo->criteria_info.inference_component_id << "\n";
    std::cout << "    Min Infer Conf  : " << pInfo->criteria_info.min_inference_confidence << "\n";
    std::cout << "    Min Track Conf  : " << pInfo->criteria_info.min_tracker_confidence << "\n";
    std::cout << "    Infer Done Only : " << pInfo->criteria_info.inference_done_only << "\n";
    std::cout << "    Min Width       : " << pInfo->criteria_info.min_width << "\n";
    std::cout << "    Min Height      : " << pInfo->criteria_info.min_height << "\n";
    std::cout << "    Max Width       : " << pInfo->criteria_info.max_width << "\n";
    std::cout << "    Max Height      : " << pInfo->criteria_info.max_height << "\n";
    std::cout << "    Interval        : " << pInfo->criteria_info.interval << "\n";
}

int main(int argc, char** argv)
{
    DslReturnType retval = DSL_RESULT_FAILURE;
    using namespace YML_VARIABLE;

    Yaml::Node root;
    Yaml::Parse(root, "run.yml");

    auto str2wstr = [&root](const std::string &key){
        auto suri = root[key].As<std::string>();
        return std::wstring(suri.begin(), suri.end());
    };

    uri_cnt = root["uri_cnt"].As<int>();
    std::wstring inputs = str2wstr("inputs");
    std::vector<std::wstring> uri;
    for(int i=0; i<uri_cnt; i++) {
        uri.emplace_back(str2wstr("uri"+std::to_string(i)));
    }
    std::wstring rtsp_url = str2wstr("rtsp_url");
    std::wstring preproc_config = str2wstr("preprocess");
    std::wstring primary_infer_config_file = str2wstr("infer");
    std::wstring primary_model_engine_file = str2wstr("model");
    std::wstring tracker_config_file = str2wstr("trk_cfg");
    std::wstring sink = str2wstr("sink");
    
    vector_reserve_size = root["vector_reserve_size"].As<int>();
    class_agnostic = root["class_agnostic"].As<bool>();
    preprocessing = root["preprocessing"].As<bool>();
    tracking = root["tracking"].As<bool>();
    postprocessing = root["postprocessing"].As<bool>();
    postprocess = str2wstr("postprocess");
    match_metric = str2wstr("match_metric");
    match_threshold = root["match_threshold"].As<float>();
    num_labels = root["num_labels"].As<int>();
    batch_size = root["batch_size"].As<int>();
    interval = root["interval"].As<int>();
    trk = str2wstr("trk");
    trk_width = root["trk_width"].As<int>();
    trk_height = root["trk_height"].As<int>();
    on_display_screen = root["on_display_screen"].As<bool>();
    window_width = root["window_width"].As<int>();
    window_height = root["window_height"].As<int>();
    font_size = root["font_size"].As<int>();
    bbox_border_size = root["bbox_border_size"].As<int>();
    ode = root["ode"].As<bool>();
    perf = root["perf"].As<bool>();
    monitoring = root["monitoring"].As<bool>();
    repeat_video = root["repeat_video"].As<bool>();
    logging = root["logging"].As<bool>();

    auto DSL_NMP_PROCESS_METHOD = (postprocess == L"NMM") ? 
                                    DSL_NMP_PROCESS_METHOD_MERGE : DSL_NMP_PROCESS_METHOD_SUPRESS;
    auto DSL_NMP_MATCH_METHOD = (match_metric == L"IOS") ? 
                                    DSL_NMP_MATCH_METHOD_IOS : DSL_NMP_MATCH_METHOD_IOU;

    // Since we're not using args, we can Let DSL initialize GST on first call    
    while(true) 
    {    
        ReportData report_data(0, 12);
        
        retval = dsl_pph_meter_new(L"meter-pph", 1, dsl_pph_meter_cb, &report_data);
        if (retval != DSL_RESULT_SUCCESS) break;

        //```````````````````````````````````````````````````````````````````````````````````
        // Create a new Non Maximum Processor (NMP) Pad Probe Handler (PPH). 
        retval = dsl_pph_nmp_new(L"nmp-pph", nullptr,
            DSL_NMP_PROCESS_METHOD, DSL_NMP_MATCH_METHOD, match_threshold);
        if (retval != DSL_RESULT_SUCCESS) break;

        if (ode) {
            // Create an Any-Class Occurrence Trigger for our remove Actions
            retval = dsl_ode_trigger_occurrence_new(L"every-occurrence-trigger", DSL_ODE_ANY_SOURCE, DSL_ODE_ANY_CLASS, DSL_ODE_TRIGGER_LIMIT_NONE);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_color_custom_new(L"full-white", 1.0, 1.0, 1.0, 1.0);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_display_type_rgba_color_custom_new(L"opaque-black", 0.0, 0.0, 0.0, 0.8);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_font_new(L"verdana-bold-16-white", L"verdana bold", font_size, L"full-white");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_label_format_new(L"format-label", 
                L"verdana-bold-16-white", 
                true, 
                L"opaque-black");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_color_palette_random_new(L"random-color", num_labels, DSL_COLOR_HUE_RANDOM, DSL_COLOR_LUMINOSITY_RANDOM, 1.0, 1000);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_bbox_format_new(L"format-bbox", bbox_border_size, L"random-color", false, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            uint content_types[] = {DSL_METRIC_OBJECT_TRACKING_ID, DSL_METRIC_OBJECT_CLASS};
            retval = dsl_ode_action_label_customize_new(L"customize-label-action", content_types, sizeof(content_types)/sizeof(uint));
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_monitor_new(L"every-occurrence-monitor", ode_occurrence_monitor, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_label_offset_new(L"offset-label-action", 0, -15);
            if (retval != DSL_RESULT_SUCCESS) break;

            // output file path for the MOT Challenge File Action. 
            std::wstring file_path(L"./log.csv");
            // DSL_EVENT_FILE_FORMAT_CSV, DSL_EVENT_FILE_FORMAT_MOTC
            retval = dsl_ode_action_file_new(L"write-data-log", 
                file_path.c_str(), DSL_WRITE_MODE_TRUNCATE, 
                DSL_EVENT_FILE_FORMAT_MOTC, false);
            if (retval != DSL_RESULT_SUCCESS) break;

            if (monitoring) {
                const wchar_t* actions[] = {L"format-bbox", L"format-label", L"every-occurrence-monitor", L"customize-label-action", nullptr};
                retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            }
            else if (logging) {
                const wchar_t* actions[] = {L"format-bbox", L"format-label", L"write-data-log", L"customize-label-action", L"offset-label-action", nullptr};
                retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            }
            else {
                const wchar_t* actions[] = {L"format-bbox", L"format-label", L"customize-label-action", nullptr};
                retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            }
            if (retval != DSL_RESULT_SUCCESS) break;

            // `````````````````````````````````````````````````````````````````````````````
            // New ODE Handler to handle all ODE Triggers with their Areas and Actions
            retval = dsl_pph_ode_new(L"ode-handler");
            if (retval != DSL_RESULT_SUCCESS) break;

            // Add the two Triggers to the ODE PPH to be invoked on every frame. 
            const wchar_t* triggers[] = {L"every-occurrence-trigger", nullptr};
            retval = dsl_pph_ode_trigger_add_many(L"ode-handler", triggers);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        if (inputs == L"video") {
            // New File Source
            for(int i=0; i<uri_cnt; i++) {
                std::string component = std::string("uri-source-"+std::to_string(i));
                std::wstring v_component = std::wstring(component.begin(), component.end());
                retval = dsl_source_file_new(v_component.c_str(), uri[i].c_str(), repeat_video);
                if (retval != DSL_RESULT_SUCCESS) break;
                
                if (i==0) {
                    const wchar_t* component_names[] = 
                    {
                        v_component.c_str(),
                        NULL
                    };

                    retval = dsl_pipeline_new_component_add_many(L"pipeline",
                        component_names);
                    if (retval != DSL_RESULT_SUCCESS) break;
                }
                else {
                    retval = dsl_pipeline_component_add(L"pipeline",
                        v_component.c_str());
                    if (retval != DSL_RESULT_SUCCESS) break;
                }
                
            }
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else {
            // # For each camera, create a new RTSP Source for the specific RTSP URI    
            retval = dsl_source_rtsp_new(L"rtsp-source", rtsp_url.c_str(), DSL_RTP_ALL,     
                false, 0, 100, 2);
            if (retval != DSL_RESULT_SUCCESS)    
                return retval;

            const wchar_t* component_names[] = 
            {
                L"rtsp-source",
                NULL
            };
            
            retval = dsl_pipeline_new_component_add_many(L"pipeline", component_names);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        if (preprocessing) {
            // New Preprocessor component using the config filespec defined above.
            retval = dsl_preproc_new(L"preprocessor", preproc_config.c_str());
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"preprocessor");
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        // New Primary GIE using the filespecs defined above, with interval and Id
        retval = dsl_infer_gie_primary_new(L"primary-gie", 
            primary_infer_config_file.c_str(), primary_model_engine_file.c_str(), interval);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        if (preprocessing) {
            // **** IMPORTANT! for best performace we explicity set the GIE's batch-size 
            // to the number of ROI's defined in the Preprocessor configuraton file.
            retval = dsl_infer_batch_size_set(L"primary-gie", batch_size);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            // **** IMPORTANT! we must set the input-meta-tensor setting to true when
            // using the preprocessor, otherwise the GIE will use its own preprocessor.
            retval = dsl_infer_gie_tensor_meta_settings_set(L"primary-gie",
                true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        retval = dsl_pipeline_component_add(L"pipeline", L"primary-gie");
        if (retval != DSL_RESULT_SUCCESS) break;

        if (tracking) {
            if (trk == L"IOU") {
                retval = dsl_tracker_iou_new(L"tracker", tracker_config_file.c_str(), trk_width, trk_height);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
            else if (trk == L"KLT") {
                retval = dsl_tracker_ktl_new(L"tracker", trk_width, trk_height);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
            else if (trk == L"DCF") {
                retval = dsl_tracker_dcf_new(L"tracker", tracker_config_file.c_str(), trk_width, trk_height, true, false);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            retval = dsl_pipeline_component_add(L"pipeline", L"tracker");
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        // New OSD with text, clock and bbox display all enabled.
        if (on_display_screen) {
            retval = dsl_osd_new(L"on-screen-display", true, true, true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            if (postprocessing) {
                retval = dsl_tracker_pph_add(L"tracker", L"nmp-pph", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            if (perf) {
                retval = dsl_osd_pph_add(L"on-screen-display", L"meter-pph", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            if (ode) {
                retval = dsl_osd_pph_add(L"on-screen-display", L"ode-handler", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
            
            retval = dsl_pipeline_component_add(L"pipeline", L"on-screen-display");
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else if (tracking) {
            // Add the NMP PPH to the source pad of the Tracker
            if (postprocessing) {
                retval = dsl_tracker_pph_add(L"tracker", L"nmp-pph", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            if (perf) {
                retval = dsl_tracker_pph_add(L"tracker", L"meter-pph", DSL_PAD_SRC);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            if (ode) {
                retval = dsl_tracker_pph_add(L"tracker", L"ode-handler", DSL_PAD_SRC);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
        }

        if (sink == L"window") {
            // New Overlay Sink, 0 x/y offsets and same dimensions as Tiled Display
            retval = dsl_sink_window_new(L"window-sink", 0, 0, window_width, window_height);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"window-sink");
            if (retval != DSL_RESULT_SUCCESS) break;

            // Add the EOS listener and XWindow event handler functions defined above
            retval = dsl_pipeline_eos_listener_add(L"pipeline", eos_event_listener, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_xwindow_key_event_handler_add(L"pipeline", 
                xwindow_key_event_handler, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_xwindow_delete_event_handler_add(L"pipeline", 
                xwindow_delete_event_handler, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else if (sink == L"fake") {
            retval = dsl_sink_fake_new(L"fake-sink");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"fake-sink");
            if (retval != DSL_RESULT_SUCCESS) break;
        }
 
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
            
