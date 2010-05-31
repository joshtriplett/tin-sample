--- a/include/autoconf.hin
+++ b/include/autoconf.hin
@@ -892,4 +892,8 @@ Added missing headers after gettext upda
  */
 #	undef DISABLE_PIPELINING
 
+/* debian-specific */
+#define XFACE_ABLE
+#define NNTP_SERVER_FILE "/etc/news/server"
+
 #endif /* !TIN_AUTOCONF_H */
