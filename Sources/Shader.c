#include "pch.h"

#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/vr/vrinterface.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <assert.h>
#include <stdlib.h>

static kinc_g4_shader_t vertex_shader;
static kinc_g4_shader_t fragment_shader;
static kinc_g4_pipeline_t pipeline;
static kinc_g4_vertex_buffer_t vertices;
static kinc_g4_index_buffer_t indices;
static kinc_g4_constant_location_t wvp_location;

#define HEAP_SIZE 1024 * 1024
static uint8_t *heap = NULL;
static size_t heap_top = 0;

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static void update() {
	kinc_g4_begin(0);

	kinc_vr_interface_begin();

	for (int eye = 0; eye < 2; ++eye) {
		kinc_vr_interface_begin_render(eye);

		kinc_vr_sensor_state_t state = kinc_vr_interface_get_sensor_state(eye);
		kinc_matrix4x4_t view = state.pose.vrPose.eye;
		kinc_matrix4x4_t proj = state.pose.vrPose.projection;
		kinc_matrix4x4_t wvp = kinc_matrix4x4_multiply(&view, &proj);
		kinc_g4_set_matrix4(wvp_location, &wvp);

		kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
		kinc_g4_set_pipeline(&pipeline);
		kinc_g4_set_vertex_buffer(&vertices);
		kinc_g4_set_index_buffer(&indices);
		kinc_g4_draw_indexed_vertices();

		kinc_vr_interface_end_render(eye);
	}

	kinc_vr_interface_warp_swap();
	kinc_g4_restore_render_target();

	// kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
	// draw to desktop display...

	kinc_g4_end(0);
	kinc_g4_swap_buffers();
}

static void load_shader(const char *filename, kinc_g4_shader_t *shader, kinc_g4_shader_type_t shader_type) {
	kinc_file_reader_t file;
	kinc_file_reader_open(&file, filename, KINC_FILE_TYPE_ASSET);
	size_t data_size = kinc_file_reader_size(&file);
	uint8_t *data = allocate(data_size);
	kinc_file_reader_read(&file, data, data_size);
	kinc_file_reader_close(&file);
	kinc_g4_shader_init(shader, data, data_size, shader_type);
}

int kickstart(int argc, char **argv) {
	kinc_init("Shader", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	load_shader("shader.vert", &vertex_shader, KINC_G4_SHADER_TYPE_VERTEX);
	load_shader("shader.frag", &fragment_shader, KINC_G4_SHADER_TYPE_FRAGMENT);

	kinc_g4_vertex_structure_t structure;
	kinc_g4_vertex_structure_init(&structure);
	kinc_g4_vertex_structure_add(&structure, "pos", KINC_G4_VERTEX_DATA_FLOAT3);
	kinc_g4_pipeline_init(&pipeline);
	pipeline.vertex_shader = &vertex_shader;
	pipeline.fragment_shader = &fragment_shader;
	pipeline.input_layout[0] = &structure;
	pipeline.input_layout[1] = NULL;
	kinc_g4_pipeline_compile(&pipeline);
	wvp_location = kinc_g4_pipeline_get_constant_location(&pipeline, "wvp");

	kinc_g4_vertex_buffer_init(&vertices, 3, &structure, KINC_G4_USAGE_STATIC, 0);
	{
		float *v = kinc_g4_vertex_buffer_lock_all(&vertices);
		int i = 0;

		v[i++] = -1;
		v[i++] = -1;
		v[i++] = -1;

		v[i++] = 1;
		v[i++] = -1;
		v[i++] = -1;

		v[i++] = 0;
		v[i++] = 1;
		v[i++] = -1;

		kinc_g4_vertex_buffer_unlock_all(&vertices);
	}

	kinc_g4_index_buffer_init(&indices, 3, KINC_G4_INDEX_BUFFER_FORMAT_32BIT);
	{
		int *i = kinc_g4_index_buffer_lock(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g4_index_buffer_unlock(&indices);
	}

	kinc_start();

	return 0;
}
