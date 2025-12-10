package com.gametracker; // <-- IMPORTANT: Change this to your package

import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.common.Mod;

// The value here should match an entry in the META-INF/mods.toml file
@Mod("stattrackermod") // <-- IMPORTANT: Change YOUR_MOD_ID to your actual Mod ID
public class StatTrackerMod { // <-- IMPORTANT: Change this to your class name

    public StatTrackerMod(IEventBus modEventBus) {
        // The PlayerDataSender class uses @EventBusSubscriber, so it registers itself.
        // You don't need to do anything else here for this specific feature.
    }
}