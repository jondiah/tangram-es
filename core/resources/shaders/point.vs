#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform mat4 u_ortho;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
#ifdef TANGRAM_TEXT
uniform vec2 u_uv_scale_factor;
uniform int u_pass;
#endif

#pragma tangram: uniforms

attribute vec2 a_uv;
attribute vec2 a_position;
attribute vec2 a_screen_position;
attribute LOWP float a_alpha;
attribute LOWP float a_rotation;
attribute LOWP vec4 a_color;
#ifdef TANGRAM_TEXT
attribute LOWP vec4 a_stroke;
#else
attribute vec3 a_extrude;
#endif

varying vec4 v_color;
varying vec2 v_texcoords;
#ifdef TANGRAM_TEXT
varying float v_sdf_threshold;
#endif
varying float v_alpha;
const vec4 clipped = vec4(2.0, 0.0, 2.0, 1.0);

#pragma tangram: global

#define UNPACK_POSITION(x) (x / 4.0) // 4 subpixel precision
#define UNPACK_EXTRUDE(x) (x / 256.0)
#define UNPACK_ROTATION(x) (x / 4096.0)

void main() {

    if (a_alpha > TANGRAM_EPSILON) {

        v_alpha = a_alpha;
        v_color = a_color;

        vec2 vertex_pos = UNPACK_POSITION(a_position);

        #ifdef TANGRAM_TEXT
        v_texcoords = a_uv * u_uv_scale_factor;
        if (u_pass == 0) {
            // fill
            v_sdf_threshold = 0.5;
        } else {
            // stroke
            float stroke_width = a_stroke.a;
            v_sdf_threshold = 0.5 - stroke_width * u_device_pixel_ratio;
            v_color.rgb = a_stroke.rgb;
        }
        #else
        v_texcoords = a_uv;
        if (a_extrude.x != 0.0) {
            float dz = u_map_position.z - abs(u_tile_origin.z);
            vertex_pos.xy += clamp(dz, 0.0, 1.0) * UNPACK_EXTRUDE(a_extrude.xy);
        }
        #endif

        // rotates first around +z-axis (0,0,1) and then translates by (tx,ty,0)
        float st = sin(UNPACK_ROTATION(a_rotation));
        float ct = cos(UNPACK_ROTATION(a_rotation));
        vec2 screen_pos = UNPACK_POSITION(a_screen_position);
        vec4 position = vec4(
            vertex_pos.x * ct - vertex_pos.y * st + screen_pos.x,
            vertex_pos.x * st + vertex_pos.y * ct + screen_pos.y,
            0.0, 1.0
        );

        #pragma tangram: position

        gl_Position = u_ortho * position;

    } else {
        gl_Position = clipped;
    }

}
