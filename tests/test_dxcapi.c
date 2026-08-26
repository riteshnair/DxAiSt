// [DXAIT-TEST: dxcapi-c99]
// [DXAIT-CPU-IMPACT: none]

#include "dxait/dx_c_api.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    assert(dx_c_api_version() == DXAIT_C_API_VERSION);
    assert(dx_create_device(0u, NULL) == DX_C_STATUS_INVALID_ARGUMENT);
    assert(dx_last_error()[0] != '\0');

    dx_device* device = NULL;
    assert(dx_create_device(0u, &device) == DX_C_STATUS_OK);
    assert(device != NULL);

    dx_c_device_info_t info;
    memset(&info, 0, sizeof(info));
    info.struct_size = (uint32_t) sizeof(info);
    info.api_version = DXAIT_C_API_VERSION;
    assert(dx_device_get_info(device, &info) == DX_C_STATUS_OK);
    assert(info.backend != 0u);

    char description[256];
    assert(dx_device_desc(device, description, (uint32_t) sizeof(description)) == DX_C_STATUS_OK);
    assert(description[0] != '\0');

    dx_queue* queue = NULL;
    assert(dx_device_queue(device, &queue) == DX_C_STATUS_OK);
    assert(queue != NULL);

    dx_c_buffer_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.struct_size = (uint32_t) sizeof(desc);
    desc.api_version = DXAIT_C_API_VERSION;
    desc.bytes = 64u;
    desc.location = DX_C_BUFFER_UPLOAD;

    dx_buffer* buffer = NULL;
    assert(dx_create_buffer_ex(device, &desc, &buffer) == DX_C_STATUS_OK);
    assert(dx_buffer_size(buffer) == desc.bytes);
    assert(dx_buffer_map(buffer) != NULL);

    const uint32_t upload_value = 0x12345678u;
    uint32_t download_value = 0u;
    assert(dx_upload(device, buffer, 4u, &upload_value, sizeof(upload_value)) == DX_C_STATUS_OK);
    assert(dx_download(device, buffer, 4u, &download_value, sizeof(download_value)) == DX_C_STATUS_OK);
    assert(download_value == upload_value);
    assert(dx_upload(device, buffer, 64u, &upload_value, 1u) == DX_C_STATUS_INVALID_ARGUMENT);

    dx_c_buffer_desc_t default_desc = desc;
    default_desc.location = DX_C_BUFFER_DEFAULT;
    dx_buffer* default_buffer = NULL;
    assert(dx_create_buffer_ex(device, &default_desc, &default_buffer) == DX_C_STATUS_OK);
    assert(default_buffer != NULL);
    uint32_t default_download = 0u;
    assert(dx_upload(device, default_buffer, 8u, &upload_value, sizeof(upload_value)) == DX_C_STATUS_OK);
    assert(dx_download(device, default_buffer, 8u, &default_download, sizeof(default_download)) == DX_C_STATUS_OK);
    assert(default_download == upload_value);
    dx_destroy_buffer(default_buffer);

    {
        const float input[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float output[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        assert(dx_copy_f32(queue, input, output, 4u) == DX_C_STATUS_OK);
        assert(output[0] == 1.0f && output[3] == 4.0f);
    }
    {
        float output[3] = { 0.0f, 0.0f, 0.0f };
        assert(dx_fill_f32(queue, output, 3u, 2.5f) == DX_C_STATUS_OK);
        assert(output[0] == 2.5f && output[2] == 2.5f);
    }
    {
        const float a[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
        const float b[3] = { 1.0f, 1.0f, 1.0f };
        float c[2] = { 0.0f, 0.0f };
        assert(dx_gemm_f32(queue, a, b, c, 2u, 1u, 3u, 1.0f, 0.0f) == DX_C_STATUS_OK);
        assert(c[0] == 6.0f && c[1] == 15.0f);
    }

    dx_c_buffer_desc_t op_desc = desc;
    op_desc.bytes = 64u;
    dx_buffer* op_a = NULL;
    dx_buffer* op_b = NULL;
    dx_buffer* op_out = NULL;
    assert(dx_create_buffer_ex(device, &op_desc, &op_a) == DX_C_STATUS_OK);
    assert(dx_create_buffer_ex(device, &op_desc, &op_b) == DX_C_STATUS_OK);
    assert(dx_create_buffer_ex(device, &op_desc, &op_out) == DX_C_STATUS_OK);
    {
        const float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const float b[4] = { 2.0f, 4.0f, 6.0f, 8.0f };
        float result[4] = {};
        assert(dx_upload(device, op_a, 0u, a, sizeof(a)) == DX_C_STATUS_OK);
        assert(dx_upload(device, op_b, 0u, b, sizeof(b)) == DX_C_STATUS_OK);
        assert(dx_la_elementwise(device, queue, op_out, op_a, op_b, 4u, 0, 1.0f, 0.0f) == DX_C_STATUS_OK);
        assert(dx_download(device, op_out, 0u, result, sizeof(result)) == DX_C_STATUS_OK);
        assert(result[0] == 3.0f && result[3] == 12.0f);
        assert(dx_la_elementwise(device, queue, op_out, op_a, op_b, 4u, 3, 1.0f, 0.0f) == DX_C_STATUS_OK);
        assert(dx_la_activation(device, queue, op_out, op_a, 4u, 0, 0.01f) == DX_C_STATUS_OK);
    }
    {
        const float input[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        const float gamma[2] = { 1.0f, 1.0f };
        float output[4] = {};
        dx_c_buffer_desc_t rms_desc = desc;
        rms_desc.bytes = sizeof(input);
        dx_buffer* rms_in = NULL;
        dx_buffer* rms_out = NULL;
        dx_buffer* rms_gamma = NULL;
        assert(dx_create_buffer_ex(device, &rms_desc, &rms_in) == DX_C_STATUS_OK);
        assert(dx_create_buffer_ex(device, &rms_desc, &rms_out) == DX_C_STATUS_OK);
        dx_c_buffer_desc_t gamma_desc = desc;
        gamma_desc.bytes = sizeof(gamma);
        assert(dx_create_buffer_ex(device, &gamma_desc, &rms_gamma) == DX_C_STATUS_OK);
        assert(dx_upload(device, rms_in, 0u, input, sizeof(input)) == DX_C_STATUS_OK);
        assert(dx_upload(device, rms_gamma, 0u, gamma, sizeof(gamma)) == DX_C_STATUS_OK);
        assert(dx_la_rmsnorm(device, queue, rms_out, rms_in, rms_gamma, 2u, 2u, 1.0e-5f) == DX_C_STATUS_OK);
        assert(dx_download(device, rms_out, 0u, output, sizeof(output)) == DX_C_STATUS_OK);
        assert(output[0] > 0.4f && output[0] < 0.7f);
        dx_destroy_buffer(rms_gamma);
        dx_destroy_buffer(rms_out);
        dx_destroy_buffer(rms_in);
    }
    {
        const float input[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float output[4] = {};
        assert(dx_upload(device, op_a, 0u, input, sizeof(input)) == DX_C_STATUS_OK);
        assert(dx_la_softmax(device, queue, op_out, op_a, 2u, 2u) == DX_C_STATUS_OK);
        assert(dx_download(device, op_out, 0u, output, sizeof(output)) == DX_C_STATUS_OK);
        assert(output[0] < output[1] && output[2] < output[3]);
        assert(dx_la_reduce(device, queue, op_out, op_a, 2u, 2u, 0) == DX_C_STATUS_OK);
        assert(dx_download(device, op_out, 0u, output, sizeof(float) * 2u) == DX_C_STATUS_OK);
        assert(output[0] == 3.0f && output[1] == 7.0f);
    }
    dx_destroy_buffer(op_out);
    dx_destroy_buffer(op_b);
    dx_destroy_buffer(op_a);

    assert(dx_wait(device) == DX_C_STATUS_OK);
    dx_destroy_buffer(buffer);
    dx_destroy_queue(queue);
    dx_destroy_device(device);
    return 0;
}
