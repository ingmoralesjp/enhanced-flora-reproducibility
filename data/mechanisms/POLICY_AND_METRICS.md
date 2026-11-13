# Auditoría de política y métricas complementarias — Enhanced FLoRa

Versión del export: `platform-paper-mechanisms-v1`. La [campaña congelada](../../results/fidelity-ablation-corrected-v0.2.0-dev/state/campaign-manifest.json) tiene 900/900 ejecuciones Enhanced aceptadas. Este paquete no inicia simulaciones ni mezcla la referencia oficial FLoRa 1.1 (100 intentos, 92 aceptados, ocho fallidos). `enhanced-mechanisms.csv` se une al [export base](../platform-paper-run-level-v1/enhanced-runs.csv) mediante `run_id`, y repite en cada fila las claves de pareamiento, aceptación y hashes de origen. El script `export_mechanisms.py` valida los 900 escalares originales; su SHA-256 y los hashes de entradas/salida están en `export-manifest.json`.

## Política que sí se ejecutó

| Población | Política del servidor | Política del nodo | Asignación |
| --- | --- | --- | --- |
| Estática | `lorawanReference`, código 1 | `network`, código 0 | todos los nodos |
| RandomWaypoint | `lorawanReference`, código 1 | `network`, código 0 | todos los nodos |
| Mixta | `lorawanReference`, código 1 | `network`, código 0 | tanto la mitad móvil como la estática |

El INI ejecutado es `dist/enhanced-flora-0.2.0-dev-rc11/simulations/examples/Analisis-fidelity-ablation.ini` (SHA-256 `b6f481e96606dfc56311cc317de5e4f9f37cb239139279362ff9e65fa3aebf9d`), incluido desde `fidelity-ablation.ini` (SHA-256 `8f4e78d918ecdf750f55daf5dadd653945c450a24790be6a2be63ca5afc5abd9`). El bloque `FidelityAblationBase` fija `adrAlgorithm="lorawanReference"`; las configuraciones Mobile solo cambian `mobile=true`, y Mixed fija `mixedMobility=true`, `mobileNodeFraction=0.5`. `SimpleLoRaApp.ned` deja `clientAdrMode="network"` salvo override; no hay override para F0–F5. La inspección de los escalares confirma `adrAlgorithmCode=1` en los 900 servidores y `clientAdrModeCode=0` en los 495 000 nodos. El manifiesto de campaña tiene SHA-256 `1bab8d90da8df01458c65a939a7f2c5ac6d769b54185272bcb471848f774dc3a` y registra el binario ejecutado `libflora.so` con SHA-256 `a83fbfd8ee4c9557fd1b0491fb390ceaf880eeed1bd7cbd4d95108cee4bbfcb5`.

En el export base, `control_policy` fue escrito como valor constante por el exportador, **no** leído de cada escalar; la auditoría independiente de esta tabla confirma que su valor era correcto. La etiqueta `legacy` de F0 se refiere a aproximaciones internas de fidelidad, no a FLoRa 1.1 sin modificar ni a BADR. Si una versión del manuscrito atribuye BADR a móviles, esa atribución no describe estas 900 ejecuciones.


La instantánea de código del manifiesto de distribución contiene `NetworkServerApp.cc` (SHA-256 `78b9dada8e4e85babb3280c6f645f0d767ced39233df98ce86d2367db1b8fdd1`), `SimpleLoRaApp.cc` (`78bbbdbb5ba05c9b157d25cbfa85ad6146e00a139cec6322b889a4799f208013`) y `SimpleLoRaApp.ned` (`5ae636b05595fb27d85937d404b16a4a6fd608fb270e1cc1a0b6881fd190a0b9`).
### `lorawanReference` operativo

El gateway pasa al servidor el **SNIR mínimo del paquete** mediante `frame->getSNIR()`; el servidor lo convierte a dB y lo llama `bestSnr`. Con un gateway, la selección del mejor gateway es trivial. Para cada nodo conserva las 20 observaciones recibidas más recientes (`adrWindowSize=20`) y toma el máximo (`adrMethod="max"`). El margen es `SNIR_max - requiredSnr(SF_actual) - 15 dB`; los umbrales SF7–SF12 son −7.5, −10, −12.5, −15, −17.5 y −20 dB. El margen se divide entre 2.5 dB por paso; SF baja primero hacia SF7 y luego baja potencia en pasos de 3 dB. Las evaluaciones ordinarias ocurren tras 20 uplinks **recibidos** desde la decisión previa, o anticipadamente si llega `ADRACKReq`. Un resultado sin cambios no genera comando, salvo respuesta forzada a `ADRACKReq`. El nodo conserva además la recuperación ADR_ACK local (`evaluateADRinNode=true`), que no es Blind ADR. Esta ruta es una variante de referencia implementada en Enhanced FLoRa; **no** es el código ADR upstream de FLoRa 1.1, que en su propia ruta usa `round(SNRmargin/3)`.

### BADR disponible en el código, pero no ejecutado aquí

`clientAdrMode="blind"` aplicaría `[12, 10, 10, 7, 7, 7]` a seis **eventos sucesivos de datos** y repetiría el ciclo. Esta implementación lo proyecta sobre el mismo reloj de tráfico `exponential(1000s)`; **no** programa exactamente seis transmisiones espaciadas dentro de cada hora ni un flujo adicional. La secuencia solo selecciona SF; no fija por sí misma potencia. El paquete de aplicación es no confirmado por defecto (`isconfirmedMSG=false`) y no se activa confirmación en estas configuraciones. El código no restringe `blind` a nodos móviles: eso requeriría un override de configuración que no existe en F0–F5. Por tanto, no se infiere un resultado BADR de este CSV. Los escalares MAC heredados `numRetry` no se incrementan en esta implementación y no constituyen un conteo válido de reintentos.

## Diccionario de `enhanced-mechanisms.csv`

Las primeras columnas, desde `run_id` hasta `unique_uplinks_received`, repiten las definiciones del [diccionario base](../platform-paper-run-level-v1/DATA_DICTIONARY.md). `seed` es el índice OMNeT++ `seed-set`; el par se identifica mediante `pair_id`, no solo por seed. Los contadores de 10 días comienzan tras el warm-up de 2 días del horizonte total de 12. `source_hash` identifica el binario registrado y `config_hash` el INI de entrada; `included_config_hash` identifica el INI que contiene los bloques F0–F5.

| Campo(s) | Definición, unidad, ventana y denominador |
| --- | --- |
| `server_adr_algorithm_code`, `device_client_adr_mode_code`, `device_client_adr_mode_count` | 1=`lorawanReference`, 0=`network`; el último confirma cuántos nodos fueron auditados por ejecución. |
| `paper_mean_of_node_airtime_means_s_12d` | Métrica **exacta de las figuras**: media no ponderada entre nodos de sus `meanTimeOnAir`, en segundos, con transmisiones de los 12 días. No es una media global por uplink. |
| `phy_uplink_count_12d`, `phy_uplink_total_airtime_s_12d`, `mean_airtime_per_phy_uplink_s_12d` | Sumatorios de `transmissionCount` y `totalTimeOnAir` de transmisores ED; el último es total de segundos / transmisiones ED durante 12 días. El tráfico de datos no confirmado no añade heartbeat en esta campaña (`heartbeatInterval=0`). No existe aquí la desagregación ToA de solo 10 días. |
| `gateway_below_sensitivity_noncollision_10d`, `gateway_low_snir_loss_10d`, `gateway_collision_loss_10d` | Eventos registrados por el receptor del único gateway después del warm-up: bajo sensibilidad sin colisión, fallo de umbral SNIR y colisión. Son conteos de eventos PHY, no una partición exhaustiva de cada uplink no entregado. |
| `gateway_collision_below_sensitivity_10d`, `gateway_collision_above_sensitivity_10d`, `gateway_total_below_sensitivity_10d` | Escalares PHY adicionales del gateway. Las dos categorías de colisión suman `gateway_collision_loss_10d`; el total bajo sensibilidad puede incluir eventos colisionados. |
| `gateway_other_phy_loss_scalar_inactive` | `otherPhyLosses` registrado, pero sin instrucción que lo incremente en esta versión; vale cero en las 900 filas y **no demuestra** ausencia de otras pérdidas. |
| `paper_below_sensitivity_events_per_sent_10d`, `paper_low_snir_events_per_sent_10d`, `paper_collision_events_per_sent_10d` | Razones reproducidas de las figuras: conteo PHY respectivo / `unique_uplinks_sent` de los 10 días. Son incidencias por envío de aplicación, **no** probabilidades exclusivas ni tasas sobre intentos de recepción PHY. |
| `server_adr_commands_issued_10d` | Suma de `Send ADR for node` del servidor; incrementa cuando se emite un TXCONFIG tras warm-up. No indica transmisión RF ni recepción. |
| `device_downlinks_delivered_to_app_10d` | Suma por nodo de `receivedADRCommands`; cuenta downlinks que alcanzaron la aplicación tras warm-up **antes** de comprobar que sean `TXCONFIG` y antes de aplicar SF/potencia. En esta campaña de datos no confirmados, la ruta de downlink configurada es TXCONFIG, pero el contador por sí solo no demuestra cambio efectivo de parámetros. |
| `server_adr_noop_suppressed_12d` | Decisiones sin cambio suprimidas por `sendAdrCommand`, en todo el horizonte. No se suma al contador de comandos de 10 días para inferir oportunidades. |
| `gateway_downlinks_from_server_12d` | `PacketForwarder.forwardedDownlinks`: downlinks entregados desde el servidor al forwarder del gateway durante los 12 días. La exportación verifica que iguala enviados hacia radio + rechazados por duty. |
| `gateway_downlinks_forwarded_to_radio_12d`, `gateway_downlinks_rejected_duty_12d` | `GW_forwardedDown` y `GW_droppedDC` del MAC. El primero llama `sendDown(pkt)`; no prueba irradiación exitosa ni recepción en ED. |
| `gateway_phy_downlink_transmissions_12d` | `transmissionCount` del transmisor PHY del gateway: transmisiones PHY efectivamente instanciadas en 12 días. El export verifica igualdad con `GW_forwardedDown` en las 900 ejecuciones. Sigue sin demostrar recepción en ED. |
| `gateway_downlink_rejection_fraction_12d` | Rechazados / (reenviados hacia radio + rechazados), durante 12 días. |
| `gateway_airtime_over_holdoff_ratio_12d` | `GW_downlinkTimeOnAir / GW_dutyCycleHoldoff`; no es utilización del canal dividida entre los 12 días. |
| `paper_mean_sf_changes_per_node`, `paper_mean_tp_changes_per_node` | Medias de contadores de cambios por dispositivo usadas por el extractor original; abarcan el horizonte completo y no trazan cuándo ocurrió cada cambio. |

No hay contadores auditados separados de oportunidades elegibles o downlinks programados, ni una correspondencia comando↔transmisión PHY↔recepción↔cambio SF/TP. Tampoco hay series temporales por dispositivo: `vector-recording=false` en el INI. Los conteos de gateway son de 12 días mientras emisión/recepción ADR en servidor/dispositivo es de 10; no calcular una tasa de éxito de downlink mezclándolos. Una mediación causal de F4 por feedback no queda identificada con estos escalares; debe presentarse como mecanismo compatible/plausible, no probado evento por evento.

La energía de `enhanced-device-summary.csv` procede de `measurementEnergyConsumed` de cada radio ED, integra solo el intervalo [día 2, día 12] y excluye módulos de gateway/servidor. El export base coteja la suma de los nodos con `end_device_energy_mJ` en **cada** ejecución; no se reexporta aquí.
