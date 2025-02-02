/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_API_H
#define _PACOS_API_H

/* LIB API return codes. */
#define LIB_API_SUCCESS                                 0
#define LIB_API_ERROR                                  -1
#define LIB_API_SET_SUCCESS                            LIB_API_SUCCESS
#define LIB_API_SET_ERROR                              LIB_API_ERROR
#define LIB_API_SET_ERR_INVALID_VALUE                  -2
#define LIB_API_SET_ERR_MALFORMED_ADDRESS              -3
#define LIB_API_SET_ERR_UNKNOWN_OBJECT                 -4
#define LIB_API_SET_ERR_OBJECT_ALREADY_EXIST           -5
#define LIB_API_SET_ERR_INVALID_FILTER_TYPE            -6
#define LIB_API_SET_ERR_DUPLICATE_POLICY               -7
#define LIB_API_SET_ERR_INVALID_PREFIX_RANGE           -8
#define LIB_API_SET_ERR_RMAP_NOT_EXIST                 -9
#define LIB_API_SET_ERR_RMAP_INDEX_NOT_EXIST          -10
#define LIB_API_SET_ERR_RMAP_RULE_MISSING             -11
#define LIB_API_SET_ERR_RMAP_COMPILE_ERROR            -12
#define LIB_API_SET_ERR_DIFF_ACL_TYPE                 -13
#define LIB_API_SET_ERR_OOM                           -14
#define LIB_API_SET_ERR_EXCEED_LIMIT                  -15
#define LIB_API_SET_ERR_DIFF_ACL_TYPE_PACOS_EXT       -16
#define LIB_API_SET_ERR_ACL                           (-17)
#define LIB_API_SET_ERR_ACL_CREATION                  (-18)
#define LIB_API_SET_ERR_ACL_DUPLICATE                 (-19)
#define LIB_API_SET_ERR_ACL_DELETED                   (-20)
#define LIB_API_SET_ERR_ACL_FILTER_DELETED            (-21)
#define LIB_API_SET_ERR_ACL_FOUND_OTHER_FILTER        (-22)
#define LIB_API_SET_ERR_ACL_FOUND_OTHER_TYPE          (-23)
#define LIB_API_SET_ERR_BAD_KERNEL_RULE               (-24)
#define LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH        (-25)         

#ifdef HAVE_PBR
#define LIB_API_SET_ERR_CANT_SET_SECONDARY_FIRST      -17
#define LIB_API_SET_ERR_CANT_DEL_PRIMARY              -18  
#endif /* HAVE_PBR */
#define LIB_API_SET_ERR_NEXTHOP_NOT_VALID             -19

/* API filtering type. */
#define LIB_API_FILTER_TYPE_DENY                        0
#define LIB_API_FILTER_TYPE_PERMIT                      1

/* Route map's type. */
#define LIB_API_RMAP_PERMIT                             0
#define LIB_API_RMAP_DENY                               1


/* CLI-APIs for access-list. */
int access_list_standard_set (struct apn_vr *, char *, int, char *, char *);
int access_list_standard_unset (struct apn_vr *, char *, int, char *, char *);
int access_list_extended_set (struct apn_vr *, char *, int, char *, char *,
                              char *, char *);
int access_list_extended_unset (struct apn_vr *, char *, int, char *, char *,
                                char *, char *);
int access_list_pacos_set (struct apn_vr *, char *, int, afi_t, char *);
int access_list_pacos_unset (struct apn_vr *, char *, int, afi_t, char *);
int access_list_pacos_exact_set (struct apn_vr *, char *, int, afi_t, char *);
int access_list_pacos_exact_unset (struct apn_vr *, char *, int, afi_t,
                                   char *);
int access_list_unset_by_name (struct apn_vr *, afi_t, char *);
int access_list_remark_set (struct apn_vr *, afi_t, char *, char *);
int access_list_remark_unset (struct apn_vr *, afi_t, char *);

/* CLI-APIs for prefix-list. */
int prefix_list_entry_set (struct apn_vr *, afi_t, char *,
                           u_int32_t, int, char *, u_int32_t, u_int32_t);
int prefix_list_unset (struct apn_vr *, afi_t, char *);
int prefix_list_entry_unset (struct apn_vr *, afi_t, char *,
                             u_int32_t, int, char *, u_int32_t, u_int32_t);
int prefix_list_sequence_number_set (struct apn_vr *, afi_t);
int prefix_list_sequence_number_unset (struct apn_vr *, afi_t);
int prefix_list_description_set (struct apn_vr *, afi_t, char *, char *);
int prefix_list_description_unset (struct apn_vr *, afi_t, char *);

/* CLI-APIs for route-map. */
int route_map_index_set (struct apn_vr *, char *, int, int);
int route_map_index_unset (struct apn_vr *, char *, int, int);
int route_map_unset (struct apn_vr *, char *);

int route_map_match_interface_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_interface_unset (struct apn_vr *, char *, int, int,
                                     char *);

int route_map_match_metric_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_metric_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_metric_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_metric_unset (struct apn_vr *, char *, int, int, char *);

int route_map_match_ip_address_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_ip_address_unset (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_match_ip_address_prefix_list_set (struct apn_vr *, char *, int,
                                                int, char *);
int route_map_match_ip_address_prefix_list_unset (struct apn_vr *, char *, int,
                                                  int, char *);
int route_map_match_ip_peer_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_ip_peer_unset (struct apn_vr *, char *, int, int,
                                   char *);
int route_map_match_ipv6_peer_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_ipv6_peer_unset (struct apn_vr *, char *, int, int,
                                   char *);
int route_map_match_ip_nexthop_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_ip_nexthop_unset (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_match_ip_nexthop_prefix_list_set (struct apn_vr *, char *, int,
                                                int, char *);
int route_map_match_ip_nexthop_prefix_list_unset (struct apn_vr *, char *, int,
                                                  int, char *);
int route_map_set_ip_peer_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_ip_peer_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_ip_nexthop_set (struct apn_vr *, char *, int, int, char *, 
                                  s_int16_t, char *);
int route_map_set_ip_nexthop_unset (struct apn_vr *, char *, int, int, char *,
                                    s_int16_t, char *);

#ifdef HAVE_IPV6
int route_map_match_ipv6_address_set (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_match_ipv6_address_unset (struct apn_vr *, char *, int, int,
                                        char *);
int route_map_match_ipv6_address_prefix_list_set (struct apn_vr *, char *, int,
                                                  int, char *);
int route_map_match_ipv6_address_prefix_list_unset (struct apn_vr *, char *,
                                                    int, int, char *);
int route_map_match_ipv6_nexthop_set (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_match_ipv6_nexthop_unset (struct apn_vr *, char *, int, int,
                                        char *);
int
route_map_match_ipv6_nexthop_prefix_list_set (struct apn_vr *, char *,
                                              int , int , char *);
int
route_map_match_ipv6_nexthop_prefix_list_unset (struct apn_vr *, char *,
                                                int , int , char *);
int route_map_set_ipv6_nexthop_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_ipv6_nexthop_unset (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_set_ipv6_nexthop_local_set (struct apn_vr *, char *, int, int,
                                          char *);
int route_map_set_ipv6_nexthop_local_unset (struct apn_vr *, char *, int, int,
                                            char *);
#endif /* HAVE_IPV6 */

int route_map_set_vpnv4_nexthop_set (struct apn_vr *, char *, int, int,
                                     char *);
int route_map_set_vpnv4_nexthop_unset (struct apn_vr *, char *, int, int,
                                       char *);

int route_map_match_tag_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_tag_unset (struct apn_vr *, char *, int, int, char *);
int route_map_match_route_type_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_route_type_unset (struct apn_vr *, char *, int, int,
                                      char *);
int route_map_set_tag_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_tag_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_metric_type_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_metric_type_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_level_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_level_unset (struct apn_vr *, char *, int, int, char *);

int route_map_match_as_path_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_as_path_unset (struct apn_vr *, char *, int, int, char *);
int route_map_match_origin_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_origin_unset (struct apn_vr *, char *, int, int, char *);
int route_map_match_community_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_community_unset (struct apn_vr *, char *, int, int,
                                     char *);
int route_map_match_ecommunity_set (struct apn_vr *, char *, int, int, char *);
int route_map_match_ecommunity_unset (struct apn_vr *, char *, int, int,
                                     char *);
int route_map_set_as_path_prepend_set (struct apn_vr *, char *, int, int,
                                       char *);
int route_map_set_as_path_prepend_unset (struct apn_vr *, char *, int, int,
                                         char *);
int route_map_set_origin_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_origin_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_local_preference_set (struct apn_vr *, char *, int, int,
                                        char *);
int route_map_set_local_preference_unset (struct apn_vr *, char *, int, int,
                                          char *);
int route_map_set_weight_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_weight_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_atomic_aggregate_set (struct apn_vr *, char *, int, int);
int route_map_set_atomic_aggregate_unset (struct apn_vr *, char *, int, int);
int route_map_set_aggregator_as_set (struct apn_vr *, char *, int, int,
                                     char *);
int route_map_set_aggregator_as_unset (struct apn_vr *, char *, int, int,
                                       char *);
int route_map_set_originator_id_set (struct apn_vr *, char *, int, int,
                                     char *);
int route_map_set_originator_id_unset (struct apn_vr *, char *, int, int,
                                       char *);
int route_map_set_community_delete_set (struct apn_vr *, char *, int, int,
                                        char *);
int route_map_set_community_delete_unset (struct apn_vr *, char *, int, int,
                                          char *);
int route_map_set_community_set (struct apn_vr *, char *, int, int, char *,
                                 int);
int route_map_set_community_unset (struct apn_vr *, char *, int, int, char *);
int route_map_set_ext_community_rt_set (struct apn_vr *, char *, int, int,
                                        char *);
int route_map_set_ext_community_rt_unset (struct apn_vr *, char *, int, int,
                                          char *);
int route_map_set_ext_community_soo_set (struct apn_vr *, char *, int, int,
                                         char *);
int route_map_set_ext_community_soo_unset (struct apn_vr *, char *, int, int,
                                           char *);
int route_map_set_dampening_set (struct apn_vr *, char *, int, int, char *);
int route_map_set_dampening_unset (struct apn_vr *, char *, int, int, char *);

int lib_api_client_send_preserve (struct lib_globals *zg,
				  u_int32_t preserve_time);
int lib_api_client_send_preserve_with_val (struct lib_globals *zg,
					   u_int32_t, u_char *, u_int16_t);
int lib_api_client_send_stale_remove (struct lib_globals *zg, afi_t, safi_t);

/* vty return error function. */
int lib_vty_return (struct cli *, int);

#endif /* _PACOS_API_H */
