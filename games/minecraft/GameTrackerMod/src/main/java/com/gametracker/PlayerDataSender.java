package com.gametracker; // <-- IMPORTANT: Change this to your package

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.logging.LogUtils;
import net.minecraft.client.Minecraft;
import net.minecraft.core.Direction;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.entity.vehicle.AbstractMinecart;
import net.minecraft.world.entity.vehicle.Boat;
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
import java.util.stream.IntStream;

/**
 * This class is responsible for sending detailed player data from the Minecraft client
 * to an external Python server every second.
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
                // --- Gather Player Data ---

                // 1. Basic Info
                String playerName = player.getName().getString();

                // 2. FPS (Frames Per Second)
                // This is a client-side metric from the Minecraft game instance.
                int fps = mc.getFps();

                // 3. Date and Time
                // Using Java's modern time API for a clean timestamp.
                String dateTime = LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME);

                // 4. Health and Hunger
                float health = player.getHealth();
                int foodLevel = player.getFoodData().getFoodLevel();
                float saturationLevel = player.getFoodData().getSaturationLevel();

                // 5. Location and Movement
                double x = player.getX();
                double y = player.getY();
                double z = player.getZ();
                // Momentum is the current velocity vector.
                String momentum = String.format("%.2f,%.2f,%.2f", player.getDeltaMovement().x, player.getDeltaMovement().y, player.getDeltaMovement().z);

                // 6. Player Facing Direction
                // This gets the cardinal direction the player is looking at.
                Direction facing = player.getDirection();
                String facingName = facing.getName();

                // 7. View and Selected Item/Slot
                // Yaw (horizontal rotation) and Pitch (vertical rotation)
                float yaw = mc.player != null ? mc.player.getYRot() : 0;
                float pitch = mc.player != null ? mc.player.getXRot() : 0;

                // The inventory slot the player has currently selected (0-8).
                int selectedSlot = player.getInventory().selected;
                // The ItemStack in the selected hotbar slot.
                ItemStack selectedItem = player.getInventory().getItem(selectedSlot);
                String selectedItemName = selectedItem.getItem().getDescription().getString();
                int selectedItemCount = selectedItem.getCount();

                // 8. Player Inventory
                // We serialize the entire main inventory (36 slots) into a JSON array.
                List<String> inventoryJson = new ArrayList<>();
                for (int i = 0; i < player.getInventory().getContainerSize(); i++) {
                    ItemStack stack = player.getInventory().getItem(i);
                    if (!stack.isEmpty()) {
                        // Format: "slot_id:item_name:count"
                        inventoryJson.add(String.format("%d:%s:%d", i, stack.getItem().getDescription().getString(), stack.getCount()));
                    }
                }
                String inventoryString = String.join(",", inventoryJson);

                // 9. Ride State
                boolean isRiding = player.isPassenger();
                String rideState = "walking";
                String rideVehicleType = "none";
                if (isRiding) {
                    Entity vehicle = player.getVehicle();
                    if (vehicle != null) {
                        rideVehicleType = vehicle.getType().getDescription().getString();
                        if (vehicle instanceof Boat) {
                            rideState = "in_boat";
                        } else if (vehicle instanceof AbstractMinecart) {
                            rideState = "in_minecart";
                        } else {
                            rideState = "riding_entity";
                        }
                    }
                }

                // --- Format as JSON String ---
                // Using String.format for simplicity. For complex objects, a library like Gson is recommended.
                String jsonData = String.format(
                        "{" +
                                "\"playerName\":\"%s\"," +
                                "\"fps\":%d," +
                                "\"dateTime\":\"%s\"," +
                                "\"health\":%.1f," +
                                "\"location\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}," +
                                "\"hunger\":%d," +
                                "\"saturation\":%.1f," +
                                "\"inventory\":[%s]," +
                                "\"view\":{\"yaw\":%.2f,\"pitch\":%.2f}," +
                                "\"selectedSlot\":%d," +
                                "\"selectedItem\":\"%s\"," +
                                "\"selectedItemCount\":%d," +
                                "\"facing\":\"%s\"," +
                                "\"momentum\":\"%s\"," +
                                "\"isRiding\":%b," +
                                "\"rideState\":\"%s\"," +
                                "\"rideVehicleType\":\"%s\"" +
                                "}",
                        playerName, fps, dateTime, health, x, y, z, foodLevel, saturationLevel,
                        inventoryJson.stream().map(s -> "\"" + s + "\"").collect(Collectors.joining(",")),
                        yaw, pitch, selectedSlot, selectedItemName, selectedItemCount,
                        facingName, momentum, isRiding, rideState, rideVehicleType
                );

                // --- Send Data ---
                try (Socket socket = new Socket(PYTHON_SERVER_HOST, PYTHON_SERVER_PORT)) {
                    socket.setSoTimeout(2000);
                    OutputStream output = socket.getOutputStream();
                    PrintWriter writer = new PrintWriter(output, true);
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
                // Catch any exception during data gathering to prevent the thread from crashing.
                LOGGER.error("[DataSender] An error occurred while gathering player data.", e);
            }
        });

        networkThread.setDaemon(true);
        networkThread.start();
    }
}