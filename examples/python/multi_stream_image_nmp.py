import sys
import time
from dsl import *
import pyds

coco_json = []

multi_image_source_dir = \
    '../../../datas/extract_video/cut_ffmpeg/image.%04d.jpg'

# Preprocessor config file is located under "/deepstream-services-library/test/configs"
preproc_config_file = \
    '/opt/dsl/datas/coco_s_640_cu114/config_preprocess_v5s.txt'

# Filespecs for the Primary GIE
primary_infer_config_file = \
    '/opt/dsl/datas/coco_s_640_cu114/config_infer_primary_yoloV5s.txt'

# IMPORTANT! ensure that the model-engine was generated with the config from the Preprocessing example
#  - apps/sample_apps/deepstream-preprocess-test/config_infer.txt
primary_model_engine_file = \
    '/opt/dsl/datas/coco_s_640_cu114/model_b4_gpu0_fp16.engine'

nmp_label_file = \
    '/opt/dsl/datas/coco_s_640_cu114/labels_coco.txt'

## 
# Function to be called on XWindow KeyRelease event
## 
def xwindow_key_event_handler(key_string, client_data):
    print('key released = ', key_string)
    if key_string.upper() == 'P':
        dsl_pipeline_pause('pipeline')
    elif key_string.upper() == 'R':
        dsl_pipeline_play('pipeline')
    elif key_string.upper() == 'Q' or key_string == '' or key_string == '':
        dsl_pipeline_stop('pipeline')
        dsl_main_loop_quit()

##
# Function to be called on XWindow Delete event
## 
def xwindow_delete_event_handler(client_data):
    print('delete window event')
    dsl_pipeline_stop('pipeline')
    dsl_main_loop_quit()

    
## 
# Function to be called on End-of-Stream (EOS) event
## 
def eos_event_listener(client_data):
    print('Pipeline EOS event')
    
## 
# Function to be called on every change of Pipeline state
## 
def state_change_listener(old_state, new_state, client_data):
    print('previous state = ', old_state, ', new state = ', new_state)
    if new_state == DSL_STATE_PLAYING:
        dsl_pipeline_dump_to_dot('pipeline', "state-playing")

        
def osd_sink_pad_buffer_probe(buffer, user_data):
    global coco_json
    
    # Retrieve batch metadata from the gst_buffer
    batch_meta = pyds.gst_buffer_get_nvds_batch_meta(buffer)
    l_frame = batch_meta.frame_meta_list
    while l_frame is not None:
        try:
            # Note that l_frame.data needs a cast to pyds.NvDsFrameMeta
            # The casting is done by pyds.glist_get_nvds_frame_meta()
            # The casting also keeps ownership of the underlying memory
            # in the C code, so the Python garbage collector will leave
            # it alone.
            frame_meta = pyds.glist_get_nvds_frame_meta(l_frame.data)
        except StopIteration:
            break
        
        frame_number=frame_meta.frame_num
        num_rects = frame_meta.num_obj_meta
        l_obj=frame_meta.obj_meta_list
        while l_obj is not None:
            try:
                # Casting l_obj.data to pyds.NvDsObjectMeta
                obj_meta=pyds.glist_get_nvds_object_meta(l_obj.data)
            except StopIteration:
                break
            
            bbox = [
                obj_meta.rect_params.left,
                obj_meta.rect_params.top,
                obj_meta.rect_params.width,
                obj_meta.rect_params.height,
            ]
            
            obj_coco_format = {
                "image_id": frame_number,
                "bbox": bbox,
                "score": obj_meta.confidence,
                "category_id": obj_meta.class_id,
                "category_name": obj_meta.obj_label,
                "segmentation": [],
                "iscrowd": 0,
                "area": bbox[2]*bbox[3],
            }
            
            coco_json.append(
                obj_coco_format
            )
            
            print(obj_coco_format)
            
            try: 
                l_obj=l_obj.next
            except StopIteration:
                break
            
        try:
            l_frame=l_frame.next
        except StopIteration:
            break
    
    return True


def save_json(save_path):
    global coco_json
    
    # create dir if not present
    Path(save_path).parent.mkdir(parents=True, exist_ok=True)

    # export as json
    with open(save_path, "w", encoding="utf-8") as outfile:
        json.dump(coco_json, outfile, separators=(",", ":"), cls=NumpyEncoder)

        
def main(args):

    # Since we're not using args, we can Let DSL initialize GST on first call
    while True:

        # --------------------------------------------------------------------------------
        # Step 1: We build the (final stage) Inference Pipeline with an Image-Source,
        # Preprocessor, Primary GIE, IOU Tracker, On-Screen Display, and Window Sink.
         
        retval = dsl_source_image_multi_new('multi-image-source', multi_image_source_dir, 1, 30)
        if retval != DSL_RETURN_SUCCESS:
            break
            
        # New Preprocessor component using the config filespec defined above.
        retval = dsl_preproc_new('preprocessor', preproc_config_file)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New Primary GIE using the filespecs above with interval = 0
        retval = dsl_infer_gie_primary_new('primary-gie', 
            primary_infer_config_file, primary_model_engine_file, 0)
        if retval != DSL_RETURN_SUCCESS:
            break
        
        retval = dsl_infer_batch_size_set('primary-gie', 4)
        if retval != DSL_RETURN_SUCCESS:
            break

        retval = dsl_pph_nmp_new('nmp-pph', None,
            1, 1, 0.5);
        if retval != DSL_RETURN_SUCCESS:
            break
        
        # New Custom Pad Probe Handler to call Nvidia's example callback for handling the Batched Meta Data
        retval = dsl_pph_custom_new('save-coco-format', client_handler=osd_sink_pad_buffer_probe, client_data=None)
        if retval != DSL_RETURN_SUCCESS:
            break
            
        # New OSD with text and bbox display enabled. 
        retval = dsl_osd_new('on-screen-display', 
            text_enabled=True, clock_enabled=True, bbox_enabled=True, mask_enabled=False)
        if retval != DSL_RETURN_SUCCESS:
            break
        
        retval = dsl_osd_pph_add('on-screen-display', 'nmp-pph', DSL_PAD_SINK)
        if retval != DSL_RETURN_SUCCESS:
            break
        
        # Add the custom PPH to the Sink pad of the OSD
        retval = dsl_osd_pph_add('on-screen-display', handler='save-coco-format', pad=DSL_PAD_SINK)
        if retval != DSL_RETURN_SUCCESS:
            break
            
        # New Window Sink, 0 x/y offsets and same dimensions as Tiled Display
        retval = dsl_sink_window_new('window-sink', 0, 0, 
            width=DSL_STREAMMUX_DEFAULT_WIDTH, height=DSL_STREAMMUX_DEFAULT_HEIGHT)
        if retval != DSL_RETURN_SUCCESS:
            break

        # Add all the components to our pipeline
        retval = dsl_pipeline_new_component_add_many('pipeline', components=[
            'multi-image-source', 'preprocessor', 'primary-gie',
            'on-screen-display', 'window-sink', None])
        if retval != DSL_RETURN_SUCCESS:
            break
        
        # Add the XWindow event handler functions defined above
        retval = dsl_pipeline_xwindow_key_event_handler_add("pipeline", xwindow_key_event_handler, None)
        if retval != DSL_RETURN_SUCCESS:
            break
        retval = dsl_pipeline_xwindow_delete_event_handler_add("pipeline", xwindow_delete_event_handler, None)
        if retval != DSL_RETURN_SUCCESS:
            break

        ## Add the listener callback functions defined above
        retval = dsl_pipeline_state_change_listener_add("pipeline", state_change_listener, None)
        if retval != DSL_RETURN_SUCCESS:
            break

        # Play the pipeline
        retval = dsl_pipeline_play('pipeline')
        if retval != DSL_RETURN_SUCCESS:
            break

        dsl_main_loop_run()
        retval = DSL_RETURN_SUCCESS
        break

    # Print out the final result
    print(dsl_return_value_to_string(retval))
    
#     save_dir = Path('.')
#     save_path = str(save_dir / "result.json")
#     save_json(save_path)
    
    dsl_pipeline_delete_all()
    dsl_component_delete_all()


if __name__ == "__main__":
    sys.exit(main(sys.argv))


