--- tin-1.7.3+20031209.orig/include/autoconf.hin
+++ tin-1.7.3+20031209/include/autoconf.hin
@@ -846,4 +846,8 @@
  */
 #	undef XFACE_ABLE
 
+/* debian-specific */
+#define XFACE_ABLE
+#define NNTP_SERVER_FILE "/etc/news/server"
+
 #endif /* !TIN_AUTOCONF_H */
