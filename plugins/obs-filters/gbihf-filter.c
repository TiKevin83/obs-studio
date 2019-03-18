#include <obs-module.h>
#include <graphics/matrix4.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>

#define SETTING_SwitchLeftToRight		"SwitchLeftToRight"

#define TEXT_SwitchLeftToRight			obs_module_text("SwitchLeftToRight")

struct gbihf_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *switchlefttoright_param;
	gs_eparam_t		       *pixel_size_param;

	bool				switchLeftToRight;
	struct vec2			uv_pixel_interval;
};

static const char *gbihf_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("GBIHFFilter");
}

static void gbihf_update(void *data, obs_data_t *settings)
{
	struct gbihf_filter_data *filter = data;

	bool switchLeftToRight = obs_data_get_bool(settings,
		SETTING_SwitchLeftToRight);

	filter->switchLeftToRight = (bool)switchLeftToRight;
}

static void gbihf_destroy(void *data)
{
	struct gbihf_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *gbihf_create(obs_data_t *settings, obs_source_t *context)
{
	struct gbihf_filter_data *filter =
		bzalloc(sizeof(struct gbihf_filter_data));
	char *effect_path = obs_module_file("gbihf_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->switchlefttoright_param = gs_effect_get_param_by_name(
			filter->effect, "switchLeftToRight");
		filter->pixel_size_param = gs_effect_get_param_by_name(
			filter->effect, "pixel_size");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		gbihf_destroy(filter);
		return NULL;
	}

	gbihf_update(filter, settings);
	return filter;
}

static void gbihf_render(void *data, gs_effect_t *effect)
{
	struct gbihf_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
		OBS_ALLOW_DIRECT_RENDERING))
		return;
	
	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);
	struct vec2 pixel_size;

	vec2_set(&pixel_size, 1.0f / (float)width, 1.0f / (float)height);

	gs_effect_set_vec2(filter->pixel_size_param, &pixel_size);
	gs_effect_set_bool(filter->switchlefttoright_param, &filter->switchLeftToRight);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *gbihf_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, SETTING_SwitchLeftToRight,
		TEXT_SwitchLeftToRight);
	UNUSED_PARAMETER(data);
	return props;
}

static void gbihf_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, SETTING_SwitchLeftToRight, false);
}

struct obs_source_info gbihf_filter = {
	.id = "gbihf_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = gbihf_name,
	.create = gbihf_create,
	.destroy = gbihf_destroy,
	.video_render = gbihf_render,
	.update = gbihf_update,
	.get_properties = gbihf_properties,
	.get_defaults = gbihf_defaults
};
