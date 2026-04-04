#include <t3d/t3dmodel.h>

#include "../../include/graphics/vertex_shaders.h"


void animate_flag(T3DModel* model, float transformOffset){

    // returns the global vertex buffer for a model.
    // If you have multiple models and want to only update one, you have to manually iterate over the objects.
    // see the implementation of t3d_model_draw_custom in that case.
    T3DVertPacked* verts = t3d_model_get_vertices(model);
    float globalHeight = fm_sinf(transformOffset * 2.5f) * 3.0f;

    for(uint16_t i=0; i < model->totalVertCount; ++i)
    {           
        // To better handle the interleaved vertex format,
        // t3d provides a few helper functions to access attributes
        int16_t *pos = t3d_vertbuffer_get_pos(verts, i);

        float height = 0.0f;

        if (pos[0] > 1){
            
            // water-like wobble effect
            height = fm_sinf(
                transformOffset * 4.5f
                + pos[0] * 30.11f
                + pos[2] * 20.12f
            );
        }
        pos[1] = 10.0f * height + globalHeight;

        // make lower parts darker, and higher parts brighter
        float color = height * 0.15f + 0.85f;
        uint8_t* rgba = t3d_vertbuffer_get_rgba(verts, i);
        rgba[0] = color * 255;
        rgba[1] = color * 250;
        rgba[2] = color * 250;
        rgba[3] = 0xFF;
    }

    // Don't forget to flush the cache again! (or use an uncached buffer in the first place)
    data_cache_hit_writeback(verts, sizeof(T3DVertPacked) * model->totalVertCount / 2);
}