/**
 * @file plugins.h
 * @brief Plugin system for registering runtime capabilities.
 *
 * Provides a lightweight function-pointer API for adding new commands
 * and capabilities to the chatbot without modifying core code.
 *
 * A plugin is a named handler that receives the raw user input and
 * writes a response string.  Handlers are checked in registration order.
 */
#ifndef PLUGINS_H
#define PLUGINS_H

#include <stdbool.h>
#include <stddef.h>

#define PLUGIN_MAX          32    /**< Maximum registered plugins. */
#define PLUGIN_NAME_LEN     32    /**< Max characters in plugin name. */
#define PLUGIN_TRIGGER_LEN  64    /**< Max characters in trigger keyword. */

/**
 * @brief Plugin handler function type.
 *
 * @param input    Raw user input.
 * @param out      Output buffer (response to user).
 * @param out_sz   Size of output buffer.
 * @return true if this plugin handled the request (stops further matching).
 */
typedef bool (*PluginHandler)(const char *input, char *out, size_t out_sz);

/** Opaque plugin registry. */
typedef struct PluginRegistry PluginRegistry;

/**
 * @brief Create a new plugin registry and register built-in plugins.
 * @return Newly allocated PluginRegistry, or NULL on failure.
 */
PluginRegistry *plugin_create(void);

/**
 * @brief Free the plugin registry.
 * @param reg The registry to destroy.
 */
void plugin_destroy(PluginRegistry *reg);

/**
 * @brief Register a new plugin.
 * @param reg     The plugin registry.
 * @param name    Human-readable plugin name.
 * @param trigger Substring/prefix that activates this plugin (e.g. "/help").
 * @param handler The handler function.
 * @return true on success, false if the registry is full.
 */
bool plugin_register(PluginRegistry *reg,
                     const char *name,
                     const char *trigger,
                     PluginHandler handler);

/**
 * @brief Try all plugins against the input; call the first matching handler.
 * @param reg    The plugin registry.
 * @param input  Raw user input.
 * @param out    Output buffer.
 * @param out_sz Size of output buffer.
 * @return true if a plugin handled the input.
 */
bool plugin_dispatch(PluginRegistry *reg,
                     const char *input,
                     char *out,
                     size_t out_sz);

#endif /* PLUGINS_H */
