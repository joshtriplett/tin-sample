--- tin-1.7.3+20031209.orig/include/autoconf.hin
+++ tin-1.7.3+20031209/include/autoconf.hin
@@ -846,4 +846,12 @@
  */
 #	undef XFACE_ABLE
 
+/* debian-specific */
+#define XFACE_ABLE
+#define NNTP_SERVER_FILE "/etc/news/server"
+#undef  HAVE_METAMAIL
+#define HAVE_METAMAIL 1
+#undef  PATH_METAMAIL
+#define PATH_METAMAIL "metamail"
+
 #endif /* !TIN_AUTOCONF_H */
