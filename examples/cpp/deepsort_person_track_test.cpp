#include <iostream>
#include <glib.h>
#include <X11/Xlib.h>

#include <gst/gst.h>
#include <gstnvdsmeta.h>

#include "DslApi.h"

uint PGIE_CLASS_ID_PERSON = 0;

// File path for the single File Source
std::wstring file_path1(
   L"/opt/dsl/nas_data/sample_video/deepsort_person_sample.mp4");

// std::wstring file_path1(
// 	L"/opt/nvidia/deepstream/deepstream-6.0/samples/streams/sample_1080p_h264.mp4");

std::wstring primary_infer_config_file(
    L"/opt/dsl/nas_data/COCO_DGPU/config_infer_primary_yoloV5s.txt");
std::wstring primary_model_engine_file(
    L"/opt/dsl/nas_data/COCO_DGPU/model_b1_gpu0_fp16.engine");
// std::wstring tracker_config_file(
//     L"/opt/dsl/nas_data/COCO_DGPU/config_tracker_DeepSORT_with_osnet2.yml");
// std::wstring tracker_config_file(
//     L"/opt/dsl/nas_data/COCO_DGPU/config_tracker_DeepSORT.yml");
std::wstring tracker_config_file(
    L"/opt/dsl/nas_data/FLL_DGPU/config_tracker_NvDCF_max_perf.yml");
//std::wstring tracker_config_file(
//    L"/opt/dsl/nas_data/FLL_DGPU/config_tracker_IOU.yml");

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

class ReportData
{
public:
    int m_report_count;
    int m_header_interval;

    ReportData(int count, int interval) : m_report_count(count), m_header_interval(interval) {}
};

boolean dsl_pph_meter_cb(double* session_fps_averages, double* interval_fps_averages,
    uint source_count, void* client_data)
{
    ReportData *report_data = static_cast<ReportData*>(client_data);

    if (report_data->m_report_count % report_data->m_header_interval == 0) {
        std::wstring header = L"";

        for(int i=0; i<source_count; i++) {
            header += L"FPS ";
            header += std::to_wstring(i);
            header += L" (AVG)";
        }
    }

    // Print FPS counters
    std::wstring counters = L"";

    for(int i=0; i<source_count; i++) {
        counters += std::to_wstring(interval_fps_averages[i]);
        counters += L" ";
        counters += std::to_wstring(session_fps_averages[i]);
    }

    report_data->m_report_count += 1;
    std::wcout << counters << "\n";
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
    DslReturnType retval;
    ReportData report_data(0, 12);

    // # Since we're not using args, we can Let DSL initialize GST on first call
    while(true)
    {   
        retval = dsl_ode_action_label_format_new(L"remove-label", 
            NULL, false, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;
            
        // Create a Format Bounding Box Action to remove the box border from view
        retval = dsl_ode_action_bbox_format_new(L"remove-bbox", 0,
            NULL, false, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Create an Any-Class Occurrence Trigger for our remove Actions
        retval = dsl_ode_trigger_occurrence_new(L"every-occurrence-trigger", 
            DSL_ODE_ANY_SOURCE, DSL_ODE_ANY_CLASS, DSL_ODE_TRIGGER_LIMIT_NONE);
        if (retval != DSL_RESULT_SUCCESS) break;

        const wchar_t* actions[] = {L"remove-label", L"remove-bbox", NULL};
        retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", 
            actions);
        if (retval != DSL_RESULT_SUCCESS) break;

        //```````````````````````````````````````````````````````````````````````````````````
        // Next, create an Occurrence Trigger to filter on People - defined with
        // a minimuim confidence level to eleminate most false positives

        // New Occurrence Trigger, filtering on PERSON class_id,
        retval = dsl_ode_trigger_occurrence_new(L"person-occurrence-trigger", 
            DSL_ODE_ANY_SOURCE, PGIE_CLASS_ID_PERSON, DSL_ODE_TRIGGER_LIMIT_NONE);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Set the inference done only filter. 
        retval = dsl_ode_trigger_infer_done_only_set(L"person-occurrence-trigger",
            false);
        if (retval != DSL_RESULT_SUCCESS) break;
            
        retval = dsl_ode_action_monitor_new(L"person-occurrence-monitor",
            ode_occurrence_monitor, NULL);
        if (retval != DSL_RESULT_SUCCESS) break;
            
        retval = dsl_display_type_rgba_color_custom_new(L"full-white", 1.0, 1.0, 1.0, 1.0);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        retval = dsl_display_type_rgba_color_custom_new(L"opaque-black", 0.0, 0.0, 0.0, 0.8);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_display_type_rgba_font_new(L"verdana-bold-16-white", L"verdana bold", 16, L"full-white");
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_ode_action_label_format_new(L"format-label", 
            L"verdana-bold-16-white", 
            true, 
            L"opaque-black");
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_display_type_rgba_color_palette_random_new(L"random-color", 80, DSL_COLOR_HUE_RANDOM, DSL_COLOR_LUMINOSITY_RANDOM, 1.0, 1000);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_ode_action_bbox_format_new(L"format-bbox", 3, L"random-color", false, nullptr);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        uint content_types[] = {DSL_METRIC_OBJECT_TRACKING_ID, DSL_METRIC_OBJECT_CLASS, DSL_METRIC_OBJECT_CONFIDENCE_TRACKER};
        retval = dsl_ode_action_label_customize_new(L"customize-label-action", content_types, sizeof(content_types)/sizeof(uint));
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_ode_action_label_offset_new(L"offset-label-action", 0, -15);
        if (retval != DSL_RESULT_SUCCESS) break;

        // Add the ODE Heat-Mapper to the Person Occurrence Trigger.
        const wchar_t* person_trigger_actions[] = {L"format-bbox", L"format-label", L"person-occurrence-monitor", L"customize-label-action", nullptr};
        retval = dsl_ode_trigger_action_add_many(L"person-occurrence-trigger", person_trigger_actions);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New ODE Handler to handle all ODE Triggers with their Areas and Actions
        retval = dsl_pph_ode_new(L"ode-handler");
        if (retval != DSL_RESULT_SUCCESS) break;
        
        const wchar_t* triggers[] = 
            {L"every-occurrence-trigger", L"person-occurrence-trigger", NULL};
            
        // Add the two Triggers to the ODE PPH to be invoked on every frame. 
        retval = dsl_pph_ode_trigger_add_many(L"ode-handler", 
            triggers);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_pph_meter_new(L"meter-pph", 1, dsl_pph_meter_cb, &report_data);
        if (retval != DSL_RESULT_SUCCESS) break;

        retval = dsl_source_file_new(L"uri-source-1", file_path1.c_str(), true);
        if (retval != DSL_RESULT_SUCCESS) break;

        // // New Primary GIE using the filespecs above, with interval and Id
        retval = dsl_infer_gie_primary_new(L"primary-gie", 
            primary_infer_config_file.c_str(), primary_model_engine_file.c_str(), 0);
        if (retval != DSL_RESULT_SUCCESS) break;

        // // New IOU Tracker, setting max width and height of input frame
        retval = dsl_tracker_new(L"deepsort-tracker", 
            tracker_config_file.c_str(), 960, 544);
        if (retval != DSL_RESULT_SUCCESS) break;

        // New OSD with text and bbox display enabled. 
        retval = dsl_osd_new(L"on-screen-display", true, false, true, false);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        retval = dsl_osd_pph_add(L"on-screen-display", L"meter-pph", DSL_PAD_SINK);
        if (retval != DSL_RESULT_SUCCESS) break;

        // // Add the ODE Pad Probe Handler to the Sink pad of the Tiler
        retval = dsl_osd_pph_add(L"on-screen-display", L"ode-handler", DSL_PAD_SINK);
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
