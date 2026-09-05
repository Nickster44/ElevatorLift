import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Elevator Lift Control",
  description: "Local service and operation console for the Elevator Lift controller.",
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return <html lang="en"><body>{children}</body></html>;
}
