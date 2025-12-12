package com.gametracker; // <-- IMPORTANT: Change this to your package

import com.mojang.logging.LogUtils;
import net.minecraft.client.Minecraft;
import net.minecraft.core.Direction;
import net.minecraft.network.chat.Component;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.entity.vehicle.AbstractMinecart;
import net.minecraft.world.entity.vehicle.Boat;
import net.minecraft.world.effect.MobEffect;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.item.ItemStack;
import net.neoforged.api.distmarker.Dist;
import net.neoforged.bus.api.SubscribeEvent;
import net.neoforged.fml.common.EventBusSubscriber;
import net.neoforged.neoforge.client.event.ClientTickEvent;
import org.slf4j.Logger;

import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * This class is responsible for sending detailed player data from the Minecraft client
 * to an external Python server every second, formatted to match the backend's dataFormat.py.
 */
@EventBusSubscriber(Dist.CLIENT)
public class PlayerDataSender {

    private static final Logger LOGGER = LogUtils.getLogger();

    // --- Configuration ---
    private static final String PYTHON_SERVER_HOST = "127.0.0.1";
    private static final int PYTHON_SERVER_PORT = 9999;
    // ---------------------

    private static int tickCounter = 0;
    private static final int TICKS_PER_SEND = 20;

    @SubscribeEvent
    public static void onClientTick(ClientTickEvent.Post event) {
        tickCounter++;
        if (tickCounter >= TICKS_PER_SEND) {
            tickCounter = 0;
            sendDataToPython();
        }
    }

    private static void sendDataToPython() {
        Thread networkThread = new Thread(() -> {
            Minecraft mc = Minecraft.getInstance();
            Player player = mc.player;

            if (player == null) {
                return;
            }

            try {
                // --- Gather and Format Player Data to match dataFormat.py ---

                // 1. Basic Info & FPS
                String plyrName = player.getName().getString();
                float fps = mc.getFps();

                // 2. Date and Time (split into two fields as required by DataSnap)
                LocalDateTime now = LocalDateTime.now();
                String date = now.format(DateTimeFormatter.ISO_LOCAL_DATE);
                String time = now.format(DateTimeFormatter.ofPattern("HH:mm:ss.SSSSSSSSS"));

                // 3. Health and Hunger
                float plyrHealth = player.getHealth();
                float plyrHunger = player.getFoodData().getFoodLevel();
                float plyrSat = player.getFoodData().getSaturationLevel();

                // 4. Location and Momentum (formatted as strings)
                String plyrLocation = String.format("[%.2f, %.2f, %.2f]", player.getX(), player.getY(), player.getZ());
                double plyrMomentum = player.getDeltaMovement().length();

                // 5. Player Facing Direction
                Direction facing = player.getDirection();
                String plyrFacing = String.format("(%s,)", facing.getName());

                // 6. View and Selected Item/Slot
                String plyrView = String.format("[%.2f, %.2f, %.2f]", player.getYRot(), player.getXRot(), 0.0f);
                int plyrSelectedSlot = player.getInventory().getSelectedSlot();
                ItemStack selectedItemStack = player.getInventory().getItem(plyrSelectedSlot);
                String plyrSelectedItem = selectedItemStack.getDisplayName().getString();

                // 7. Player Inventory (formatted as "ItemName:Count; ItemName:Count;")
                List<String> inventoryParts = new ArrayList<>();
                for (int i = 0; i < player.getInventory().getContainerSize(); i++) {
                    ItemStack stack = player.getInventory().getItem(i);
                    if (!stack.isEmpty()) {
                        String itemName = stack.getDisplayName().getString();
                        int itemCount = stack.getCount();
                        inventoryParts.add(String.format("%s:%d", itemName, itemCount));
                    }
                }
                String plyrInventory = String.join("; ", inventoryParts);
                if (plyrInventory.isEmpty()) {
                    plyrInventory = "None";
                }

                // 8. Player Armor and Offhand
                ItemStack mainHandItem = player.getMainHandItem();
                ItemStack offHandItem = player.getOffhandItem();
                String plyrArmor = mainHandItem.getDisplayName().getString(); // Simplified: Assuming main hand is "armor"
                String plyrOffhand = offHandItem.getDisplayName().getString();

                // 9. Player Status/Effects (formatted as "name,type,duration,amplifier; name,type,duration,amplifier;")
                List<String> statusParts = new ArrayList<>();
                for (MobEffectInstance effect : player.getActiveEffects()) {
                    // FIX 1: Call .value() on the Holder to get the MobEffect instance
                    MobEffect mobEffect = effect.getEffect().value();

                    String name = mobEffect.getDisplayName().getString();
                    // FIX 2: Use BuiltInRegistries to access the registry
                    String type = "minecraft:" + net.minecraft.core.registries.BuiltInRegistries.MOB_EFFECT.getKey(mobEffect).getPath();
                    double duration = effect.getDuration();
                    int amplifierLevel = effect.getAmplifier();
                    statusParts.add(String.format("%s,%s,%.1f,%d", name, type, duration, amplifierLevel));
                }
                String plyrStatus = String.join("; ", statusParts);
                if (plyrStatus.isEmpty()) {
                    plyrStatus = "None";
                }

                // 10. Ride State
                boolean plyrRideState = player.isPassenger();
                String plyrRideVehicle = "None";
                if (plyrRideState && player.getVehicle() != null) {
                    plyrRideVehicle = player.getVehicle().getDisplayName().getString();
                }

                // --- Format as a single JSON String ---
                // The order of keys does not matter for JSON, but keeping it consistent helps.
                String jsonData = String.format(
                        "{" +
                                "\"fps\":%.1f," +
                                "\"time\":\"%s\"," +
                                "\"date\":\"%s\"," +
                                "\"plyrName\":\"%s\"," +
                                "\"plyrLocation\":\"%s\"," +
                                "\"plyrHealth\":%.1f," +
                                "\"plyrInventory\":\"%s\"," +
                                "\"plyrArmor\":\"%s\"," +
                                "\"plyrOffhand\":\"%s\"," +
                                "\"plyrStatus\":\"%s\"," +
                                "\"plyrHunger\":%.1f," +
                                "\"plyrSat\":%.1f," +
                                "\"plyrView\":\"%s\"," +
                                "\"plyrFacing\":\"%s\"," +
                                "\"plyrSelectedSlot\":%d," +
                                "\"plyrSelectedItem\":\"%s\"," +
                                "\"plyrRideState\":%b," +
                                "\"plyrRideVehicle\":\"%s\"," +
                                "\"plyrMomentum\":%.2f" +
                                "}",
                        fps, time, date, plyrName, plyrLocation, plyrHealth, plyrInventory,
                        plyrArmor, plyrOffhand, plyrStatus, plyrHunger, plyrSat, plyrView,
                        plyrFacing, plyrSelectedSlot, plyrSelectedItem, plyrRideState,
                        plyrRideVehicle, plyrMomentum
                );

                // --- Send Data ---
                try (Socket socket = new Socket(PYTHON_SERVER_HOST, PYTHON_SERVER_PORT)) {
                    socket.setSoTimeout(2000); // 2-second timeout
                    OutputStream output = socket.getOutputStream();
                    PrintWriter writer = new PrintWriter(output, true); // true for auto-flushing
                    LOGGER.info("[DataSender] Sending data to Python server.");
                    writer.println(jsonData);
                } catch (UnknownHostException e) {
                    LOGGER.error("[DataSender] Unknown Python server host: {}", PYTHON_SERVER_HOST);
                } catch (SocketTimeoutException e) {
                    LOGGER.error("[DataSender] Connection to Python server timed out.");
                } catch (IOException e) {
                    LOGGER.error("[DataSender] Could not connect to Python server at {}:{}.", PYTHON_SERVER_HOST, PYTHON_SERVER_PORT);
                } catch (Exception e) {
                    LOGGER.error("[DataSender] An unexpected error occurred while sending data.", e);
                }

            } catch (Exception e) {
                LOGGER.error("[DataSender] An error occurred while gathering player data.", e);
            }
        });

        networkThread.setDaemon(true);
        networkThread.start();
    }
}